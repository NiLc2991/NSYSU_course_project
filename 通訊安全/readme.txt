wsl --install

sudo apt update 
sudo apt-get install mininet

chmod +x Sate.py 
sudo -E python3 Sate.py

# client請填這次 route 的起點
<client> ping -c 20 -s 64 10.0.10.2