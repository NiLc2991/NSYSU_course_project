# Your Name: 林之謙
# Your ID: B113040002

import argparse
import json
import random
import socket
import time
from typing import Any, Dict, List, Optional, Tuple
import csv
from math import floor
# Note: In this starter code, we annotate types where
# appropriate. While it is optional, both in python and for this
# course, we recommend it since it makes programming easier.

# The maximum size of the data contained within one packet
payload_size = 1200
# The maximum size of a packet including all the JSON formatting
packet_size = 1500
BDP = 30000
MAX_CWND = 0

csv_data = [
    ["Time", "CWND", "MODE"],
]

class Receiver:
    def __init__(self):
        # TODO: Initialize any variables you want here, like the receive
        # buffer, initial congestion window and initial values for the timeout
        # values
        self.rev_buffer = []
        self.rev_acks = []
        self.current_out = 0  

    def data_packet(self, seq_range: Tuple[int, int], data: str) -> Tuple[List[Tuple[int, int]], str]:
        '''This function is called whenever a data packet is
        received. `seq_range` is the range of sequence numbers
        received: It contains two numbers: the starting sequence
        number (inclusive) and ending sequence number (exclusive) of
        the data received. `data` is a binary string of length
        `seq_range[1] - seq_range[0]` representing the data.

        It should output the list of sequence number ranges to
        acknowledge and any data that is ready to be sent to the
        application. Note, data must be sent to the application
        _reliably_ and _in order_ of the sequence numbers. This means
        that if bytes in sequence numbers 0-10000 and 11000-15000 have
        been received, only 0-10000 must be sent to the application,
        since if we send the latter bytes, we will not be able to send
        bytes 10000-11000 in order when they arrive. The transport
        layer must hide hide all packet reordering and loss.

        The ultimate behavior of the program should be that the data
        sent by the sender should be stored exactly in the same order
        at the receiver in a file in the same directory. No gaps, no
        reordering. You may assume that our test cases only ever send
        printable ASCII characters (letters, numbers, punctuation,
        newline etc), so that terminal output can be used to debug the
        program.

        '''
        self.fin_ts = time.time()

        # TODO
        start, end = seq_range
        self.rev_buffer.append((start, end, data))
        self.rev_acks.append((start, end))
        
        # 重整ACKs  //在Sender重整就體驗上來說會不會比較好?
        if len(self.rev_acks) == 0: 
            return ([], '')
        self.rev_acks.sort() # 依 start 排序
        new_acks = [list(self.rev_acks[0])] # 先放進第一個方便比對
        for start, end in self.rev_acks[1:]:
            if start <= new_acks[-1][1]:          # range 重疊或相鄰
                new_acks[-1][1] = max(new_acks[-1][1], end)
                # new_acks[-1][0] 永遠小於等於 start
            else:
                new_acks.append([start, end])
        self.rev_acks = [tuple(x) for x in new_acks]

        self.rev_buffer.sort() # 依 start 排序
        out_data = ''
        i = 0
        while i < len(self.rev_buffer):
            rev_start, rev_end, rev_data = self.rev_buffer[i]

            if rev_start > self.current_out:
                i +=1
                continue          # 沒輪到它
            elif rev_end <= self.current_out:
                # 整段已傳送到 app lyr 過
                self.rev_buffer.pop(i) # 處理完移除，不遞增 idx
                continue  
            else: # 唯 current == start 或是 start < current < end
                offset = self.current_out - rev_start
                out_data = out_data + rev_data[offset:]  # 切掉重複部分
                self.current_out = rev_end
                self.rev_buffer.pop(i) # 處理完移除
        return (self.rev_acks, out_data)

    def finish(self):
        '''Called when the sender sends the `fin` packet. You don't need to do
        anything in particular here. You can use it to check that all
        data has already been sent to the application at this
        point. If not, there is a bug in the code. A real transport
        stack will deallocate the receive buffer. Note, this may not
        be called if the fin packet from the sender is locked. You can
        read up on "TCP connection termination" to know more about how
        TCP handles this.

        '''

        # TODO
        if len(self.rev_buffer)>0:
            self.rev_buffer.clear()
        pass

class Sender:
    def __init__(self, data_len: int):
        '''`data_len` is the length of the data we want to send. A real
        transport will not force the application to pre-commit to the
        length of data, but we are ok with it.

        '''
        # TODO: Initialize any variables you want here, for instance a
        # data structure to keep track of which packets have been
        # sent, acknowledged, detected to be lost or retransmitted
        self.data_len = data_len          # 總 byte 長度
        self.acks = [] # List of ranges of bytes acked
        self.in_flight = {} # {seg : class info} Sent, but not acked
        self.retx_queue = []
        self.next_seq_start = 0 # 下一個要送之資料的起始 seq
        
        self.dup_sack_cnt = 0
        self.ssthresh = 64000
        self.cwnd = packet_size

        self.mode_flag = 'slow_start'
        self.loss = 0
        self.curr = 0

        # RTO 
        self.sample_rtt = 0.0
        self.est_rtt = 0.1
        self.dev_rtt = 0.0

        self.st_ts = time.time()  # 開始時間戳
        self.fin_ts = 0.0

    class info:
        def __init__(self, seg, ack_cnt, send_ts):
            self.seg = seg
            self.ack_cnt = ack_cnt
            self.send_ts = send_ts  # 傳送時間戳

    def timeout(self):
        '''Called when the sender times out.'''
        # TODO: In addition to what you did in assignment 1, set cwnd to 1
        # packet
        self.mode_flag = 'slow_start'
        self.ssthresh = self.cwnd // 2 
        self.cwnd = packet_size
        self.dup_sack_cnt = 0

        self.retx_queue = [info.seg for info in self.in_flight.values()]# 清空 in_flight(timeout仍未ACK)
        self.retx_queue.sort()
        self.in_flight.clear()

    def ack_packet(self, sacks: List[Tuple[int, int]], packet_id: int) -> int:
        '''Called every time we get an acknowledgment. The argument is a list
        of ranges of bytes that have been ACKed. Returns the number of
        payload bytes new that are no longer in flight, either because
        the packet has been acked (measured by the unique ID) or it
        has been assumed to be lost because of dupACKs. Note, this
        number is incremental. For example, if one 100-byte packet is
        ACKed and another 500-byte is assumed lost, we will return
        600, even if 1000s of bytes have been ACKed before this.

        '''
        # any mode -timeout-> slow start   ( timeout() )
        # slow start -cwnd >= ssthresh-> congestion avoidance (got_cwnd() )
        # slow start、CA -dup 3-> fast recovery ( ack_packet() )
        # fast recovery -new ACK -> CA ( ack_packet())

        # TODO
        # 擷取goodput 結算時間戳
        if (sacks[0][1] - sacks[0][0]) == self.data_len:
            self.fin_ts = time.time()

        prev_expt = self.curr
        # 擷取 expected seq
        if sacks[0][0] == 0:
            self.curr = sacks[0][1]
        
        # 如果收到 dup ack
        if self.curr == prev_expt:
            if self.mode_flag == 'fast_recovery':
                if self.acks != sacks:
                    if self.cwnd + packet_size <= MAX_CWND:
                        self.cwnd = self.cwnd + packet_size
                    self.acks = sacks

            # slow start、CA 下收到 duplicate ACK
            else:
                self.dup_sack_cnt += 1
                if self.dup_sack_cnt >= 3:
                    self.ssthresh = self.cwnd // 2 
                    self.cwnd = self.ssthresh + 3 * packet_size
                    self.mode_flag = 'fast_recovery'
                self.acks = sacks
        # 收到 expected ACK
        else:
            self.acks = sacks
            # slow start 下收到 new ACK
            if self.mode_flag == 'slow_start':
                self.cwnd = self.cwnd + packet_size
                self.dup_sack_cnt = 0

            # fast recovery 下收到 new ACK    
            elif self.mode_flag == 'fast_recovery':
                self.cwnd = self.ssthresh
                self.mode_flag = 'congestion_avoidance'
                self.dup_sack_cnt = 0

            # congestion avoidance 下收到 new ACK
            else:
                self.cwnd = self.cwnd + floor(packet_size * packet_size/ self.cwnd)
                self.dup_sack_cnt = 0


        if packet_id in self.in_flight:
            self.sample_rtt = time.time() - self.in_flight[packet_id].send_ts
            self.est_rtt = 0.875 * self.est_rtt + 0.125 * self.sample_rtt

        freed = 0
        for id, info in list(self.in_flight.items()):
            seg = info.seg
            s, e = seg
            if any(As <= s and e <= Ae for As, Ae in self.acks):
                # 如果 seg 落在任何一個 ACK 範圍，就視為已被 ACK
                print("ACKED**: {}".format(seg))
                freed += (e - s)
                del self.in_flight[id]
                    # 如果它還在重傳 queue，也順便拿掉
                if seg in self.retx_queue:
                    self.retx_queue.remove(seg)
            # 沒有被 ACK，檢查 duplicate count
            else:
                if packet_id > id :
                    self.in_flight[id].ack_cnt += 1
                    print("DUPLICATE**: id: {}, rng: {}, Cnt: {}".format(id, seg, self.in_flight[id].ack_cnt))
                    if self.in_flight[id].ack_cnt >= 3:
                        freed += (e - s)
                        del self.in_flight[id]
                        if seg not in self.retx_queue:
                            self.retx_queue.append((s, e))

        self.retx_queue.sort()
        for seg in list(self.retx_queue):         
            s, e = seg
            if any(As <= s and e <= Ae for As, Ae in self.acks):
                self.retx_queue.remove(seg)
        if freed > 0:
            return freed
        else:
            return 0
        

    def send(self, packet_id: int) -> Optional[Tuple[int, int]]:
        '''Called just before we are going to send a data packet. Should
        return the range of sequence numbers we should send. If there
        are no more bytes to send, returns a zero range (i.e. the two
        elements of the tuple are equal). Return None if there are no
        more bytes to send, and _all_ bytes have been
        acknowledged. Note: The range should not be larger than
        `payload_size` or contain any bytes that have already been
        acknowledged

        '''
        if self.retx_queue:
            seg = self.retx_queue.pop(0)
            self.in_flight[packet_id] = self.info(seg, 0, time.time())
            print("RETRANSMIT**: {} []]PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPkt\n".format(seg))
            return seg

        # 如果沒有 gaps，則發送下個 seq
        prev = self.next_seq_start
        
        if prev < self.data_len :
            seg = (prev, min(prev + payload_size, self.data_len))
            self.next_seq_start = seg[1]
            self.in_flight[packet_id] = self.info(seg, 0, time.time())
            return seg
        elif self.acks and self.acks[0][0] == 0 and self.acks[0][1] == self.data_len:
            print("Goodput: {}", self.data_len / (self.fin_ts - self.st_ts))
            return None
        else:
            return (0,0)

        # TODO
    def get_goodput(self) -> int:
        return sum(e - s for s, e in self.good)

    def get_cwnd(self) -> int:
        # TODO
        if self.mode_flag == 'slow_start':
            if  self.cwnd >= self.ssthresh:
                self.mode_flag = 'congestion_avoidance'
        return max(self.cwnd, packet_size), self.mode_flag

    def get_rto(self) -> float:
        # TODO
        if self.sample_rtt == 0.0:
            return 1.0
        
        self.dev_rtt = 0.75 * self.dev_rtt + 0.25 * abs(self.sample_rtt - self.est_rtt)
        return max(self.est_rtt + 4 * self.dev_rtt, 0.001)

def start_receiver(ip: str, port: int):
    '''Starts a receiver thread. For each source address, we start a new
    `Receiver` class. When a `fin` packet is received, we call the
    `finish` function of that class.

    We start listening on the given IP address and port. By setting
    the IP address to be `0.0.0.0`, you can make it listen on all
    available interfaces. A network interface is typically a device
    connected to a computer that interfaces with the physical world to
    send/receive packets. The WiFi and ethernet cards on personal
    computers are examples of physical interfaces.

    Sometimes, when you start listening on a port and the program
    terminates incorrectly, it might not release the port
    immediately. It might take some time for the port to become
    available again, and you might get an error message saying that it
    could not bind to the desired port. In this case, just pick a
    different port. The old port will become available soon. Also,
    picking a port number below 1024 usually requires special
    permission from the OS. Pick a larger number. Numbers in the
    8000-9000 range are conventional.

    Virtual interfaces also exist. The most common one is `localhost',
    which has the default IP address of `127.0.0.1` (a universal
    constant across most machines). The Mahimahi network emulator also
    creates virtual interfaces that behave like real interfaces, but
    really only emulate a network link in software that shuttles
    packets between different virtual interfaces.

    '''

    receivers: Dict[str, Receiver] = {}

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
        server_socket.bind((ip, port))

        while True:
            data, addr = server_socket.recvfrom(packet_size)
            if addr not in receivers:
                receivers[addr] = Receiver()

            received = json.loads(data.decode())
            if received["type"] == "data":
                # Format check. Real code will have much more
                # carefully designed checks to defend against
                # attacks. Can you think of ways to exploit this
                # transport layer and cause problems at the receiver?
                # This is just for fun. It is not required as part of
                # the assignment.
                assert type(received["seq"]) is list
                assert type(received["seq"][0]) is int and type(received["seq"][1]) is int
                assert type(received["payload"]) is str
                assert len(received["payload"]) <= payload_size

                # Deserialize the packet. Real transport layers use
                # more efficient and standardized ways of packing the
                # data. One option is to use protobufs (look it up)
                # instead of json. Protobufs can automatically design
                # a byte structure given the data structure. However,
                # for an internet standard, we usually want something
                # more custom and hand-designed.
                sacks, app_data = receivers[addr].data_packet(tuple(received["seq"]), received["payload"])
                # Note: we immediately write the data to file
                #receivers[addr][1].write(app_data)

                # Send the ACK
                server_socket.sendto(json.dumps({"type": "ack", "sacks": sacks, "id": received["id"]}).encode(), addr)


            elif received["type"] == "fin":
                receivers[addr].finish()
                del receivers[addr]

            else:
                assert False

def start_sender(ip: str, port: int, data: str, recv_window: int, simloss: float):
    sender = Sender(len(data))
    global MAX_CWND

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client_socket:
        # So we can receive messages
        client_socket.connect((ip, port))
        # When waiting for packets when we call receivefrom, we
        # shouldn't wait more than 500ms

        # Number of bytes that we think are inflight. We are only
        # including payload bytes here, which is different from how
        # TCP does things
        inflight = 0
        packet_id  = 0
        wait = False

        while True:
            # Get the congestion window
            cwnd, mode_flag = sender.get_cwnd()
            csv_data.append([(time.time() - sender.st_ts)*1000, cwnd, mode_flag])

            MAX_CWND = max(cwnd, inflight+packet_size)

            # Do we have enough room in recv_window to send an entire
            # packet?
            if inflight + packet_size <= min(recv_window, cwnd) and not wait:
                seq = sender.send(packet_id)
                if seq is None:
                    # We are done sending
                    client_socket.send('{"type": "fin"}'.encode())
                    break
                elif seq[1] == seq[0]:
                    # No more packets to send until loss happens. Wait
                    wait = True
                    continue

                assert seq[1] - seq[0] <= payload_size
                assert seq[1] <= len(data)

                # Simulate random loss before sending packets
                if random.random() < simloss:
                    pass
                else:
                    # Send the packet
                    client_socket.send(
                        json.dumps(
                            {"type": "data", "seq": seq, "id": packet_id, "payload": data[seq[0]:seq[1]]}
                        ).encode())

                inflight += seq[1] - seq[0]
                packet_id += 1

            else:
                wait = False
                # Wait for ACKs
                try:
                    rto = sender.get_rto()
                    client_socket.settimeout(rto)
                    received_bytes = client_socket.recv(packet_size)
                    received = json.loads(received_bytes.decode())
                    assert received["type"] == "ack"

                    if random.random() < simloss:
                        continue

                    inflight -= sender.ack_packet(received["sacks"], received["id"])
                    assert inflight >= 0
                except socket.timeout:
                    inflight = 0
                    print("Timeout")
                    sender.timeout()

    with open('output.csv', 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerows(csv_data)


def main():
    parser = argparse.ArgumentParser(description="Transport assignment")
    parser.add_argument("role", choices=["sender", "receiver"], help="Role to play: 'sender' or 'receiver'")
    parser.add_argument("--ip", type=str, required=True, help="IP address to bind/connect to")
    parser.add_argument("--port", type=int, required=True, help="Port number to bind/connect to")
    parser.add_argument("--sendfile", type=str, required=False, help="If role=sender, the file that contains data to send")
    parser.add_argument("--recv_window", type=int, default=15000000, help="Receive window size in bytes")
    parser.add_argument("--simloss", type=float, default=0.0, help="Simulate packet loss. Provide the fraction of packets (0-1) that should be randomly dropped")

    args = parser.parse_args()

    if args.role == "receiver":
        start_receiver(args.ip, args.port)
    else:
        if args.sendfile is None:
            print("No file to send")
            return

        with open(args.sendfile, 'r') as f:
            data = f.read()
            start_sender(args.ip, args.port, data, args.recv_window, args.simloss)

if __name__ == "__main__":
    main()
