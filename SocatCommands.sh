tty_port="ttyS5"
port_number=3333
sudo echo "Establishing TCP link to $tty_port"
(sudo socat -d -d -d pty,link=/dev/$tty_port,b115200, tcp-l:$port_number >& SocatText.log) &
sleep 0.1
if grep -R "shutdown" SocatText.log
then
exit
fi
started=0
while [ $started -ne 1 ]
do
	if grep -R "starting data transfer" SocatText.log
	then
	started=1
	fi
done
sudo chmod o+rw /dev/ttyS5
echo "RST!" > /dev/ttyS5

