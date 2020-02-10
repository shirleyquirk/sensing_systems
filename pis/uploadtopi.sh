#!/bin/sh
if [ -z "$1" ]
then
    echo "Usage: ./install_to_pi.sh <rasp.pi.ip>"
    exit 1
fi
#try to log in with publickey
ssh -o ControlPath=None \
    -o LogLevel=INFO \
    -o PreferredAuthentications=publickey \
    -o IdentitiesOnly=yes pi@$1 exit 2>/tmp/logfile.stderr </dev/null
if [ $? != 0 ]; then
    echo "Copying publickey to raspberry pi"
    ssh pi@$1 "tee -a ~/.ssh/authorized_keys" < ~/.ssh/id_rsa.pub
fi
#try to log in as root
ssh -o ControlPath=None \
    -o LogLevel=INFO \
    -o PreferredAuthentications=publickey \
    -o IdentitiesOnly=yes root@$1 exit 2>/tmp/logfile.stderr </dev/null
if [ $? != 0 ]; then
    ssh pi@$1 "sudo mkdir /root/.ssh; sudo cp /home/pi/.ssh/authorized_keys /root/.ssh/authorized_keys"
fi
echo "installing dependencies"
ssh pi@$1 "pip3 install python-osc"
echo "copying across avahi service"
rsync playrandom.service root@$1:/etc/avahi/services/playrandom.service
echo "copying across osc server"
rsync osc_video_player root@$1:/usr/local/bin/osc_video_player
echo "copying across systemd service"
echo "ERROR NOT FOUND"
echo "copying across video files"
rsync -rc --progress ./videos/ pi@$1:~/videos

