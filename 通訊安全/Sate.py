#!/usr/bin/env python3
from mininet.net    import Mininet
from mininet.link   import TCLink
from mininet.cli    import CLI
from mininet.log    import setLogLevel
import random, time, math, os, signal, sys, atexit
from mininet.clean import cleanup

GATEWAY_COUNT = 10
MAX_HOPS      = 5
TIMEOUT       = 5.0      
COORD_RANGE   = (0, 100)
PREF_VECTOR   = (1, 0)      

atexit.register(cleanup)
for sig in (signal.SIGINT, signal.SIGTERM):
    signal.signal(sig, lambda *_: sys.exit(0))

def build_topo():
    net = Mininet(link=TCLink, controller=None)

    # sat host
    sat = net.addHost('sat', ip='10.0.255.2/24')   

    # nodes
    gws  = [net.addHost(f'gw{i}') for i in range(1, GATEWAY_COUNT+1)]

    # Build full connection between nodes
    for i in range(GATEWAY_COUNT):
        for j in range(i+1, GATEWAY_COUNT):
            net.addLink(gws[i], gws[j], cls=TCLink)

    net.addLink(gws[0], sat, cls=TCLink, delay='60ms', loss=0.1, bw=100, use_hfsc=True)

    net.start()
    net.staticArp()

    # 啟用轉發
    for gw in gws:
        gw.cmd('sysctl -w net.ipv4.ip_forward=1')

    # 分配座標
    coords = {gw.name: (random.uniform(*COORD_RANGE),
                        random.uniform(*COORD_RANGE))
              for gw in gws}

    
    return net, gws, sat, coords

# timeout 重新 routing
def choose_path(net, gws, sat, coords):
    client = random.choice(gws)
    uplink = random.choice([g for g in gws if g != client])

    # 方向加權
    cx, cy = coords[client.name]
    pdx, pdy = PREF_VECTOR
    weights = {}
    for gw in gws:
        x, y = coords[gw.name]
        dx, dy = x-cx, y-cy
        dist   = math.hypot(dx, dy) or 1
        dot    = (dx/dist)*pdx + (dy/dist)*pdy
        weights[gw] = dot + 1  

    # 建立路徑
    hop_cnt = random.randint(1, MAX_HOPS)
    pool    = [g for g in gws if g not in (client, uplink)]
    path_mid = []
    for _ in range(min(hop_cnt, len(pool))):
        total = sum(weights[g] for g in pool)
        r = random.uniform(0, total)
        upto = 0
        for g in pool:
            upto += weights[g]
            if upto >= r:
                path_mid.append(g)
                pool.remove(g)
                break

    path = [client] + path_mid + [uplink]

    print('\n=== NEW ROUTE ===')
    print(' → '.join(n.name for n in path), '→ sat')

    # 清掉舊 default route
    for gw in gws:
        gw.cmd('ip route del default || true')

    # 為 sat link 重拉線
    try:
        net.configLinkStatus(uplink.name, sat.name, 'down')
    except:
        pass
    net.addLink(uplink, sat, cls=TCLink,
                delay='60ms', loss=0.1, bw=100, use_hfsc=True)
    net.configLinkStatus(uplink.name, sat.name, 'up')

    # IP 設置
    seg = 0
    client.setIP('10.0.0.1/24')
    path[1].setIP('10.0.0.2/24', intf=path[1].connectionsTo(client)[0][1])

    for a, b in zip(path[:-1], path[1:]):
        if b is uplink: break
        seg += 1
        a.setIP(f'10.0.{seg}.1/24', intf=a.connectionsTo(b)[0][0])
        b.setIP(f'10.0.{seg}.2/24', intf=b.connectionsTo(a)[0][0])

    # uplink 與 sat
    uplink.setIP('10.0.10.1/24', intf=uplink.connectionsTo(sat)[0][0])
    sat.setIP('10.0.10.2/24',   intf=sat.connectionsTo(uplink)[0][0])

    # 指定 route
    client.cmd('ip route add default via 10.0.0.2')
    for a, b in zip(path[1:-1], path[2:]):
        a.cmd(f'ip route add default via 10.0.{seg+1}.2')
        seg += 1
    uplink.cmd('ip route add default via 10.0.10.2')
    sat.cmd('ip route add default via 10.0.10.1')

    return client, sat

# RTT 測試
def measure_rtt(client, sat):
    sat.cmd('pkill -f \"http.server\" || true')
    sat.cmd('python3 -m http.server 8000 &')
    time.sleep(0.5)
    out = client.cmd(f'timeout {TIMEOUT}s curl -s --fail http://10.0.10.2:8000')
    return bool(out.strip())
 
if __name__ == '__main__':
    setLogLevel('info')
    net, gws, sat, coords= build_topo()

    while True:
        client, sat_host = choose_path(net, gws, sat, coords)
        if measure_rtt(client, sat_host):
            print('Connection succeed\nEntering CLI。')
            break
        else:
            print(f'TIMEOUT={TIMEOUT}s Rerouting…')

    CLI(net)
    net.stop()
