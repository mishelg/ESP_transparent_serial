findSocat="$(ps -ef | grep 'socat' | grep 'root')"
findSocatPID="$(echo $findSocat | cut -d' ' -f 16)"
if ps -ef | grep 'socat' | grep 'root'
then
echo "Killing Process $findSocatPID"
else
echo "Process not found"
exit
fi

sudo kill $findSocatPID
echo "Link to ttyS5 disconnected"
