link_address=("0000:a1:00.0" "0000:a1:00.1" "0000:a1:00.2" "0000:a1:00.3" "0000:a1:00.4" "0000:a1:00.5" "0000:a1:00.6" "0000:a1:00.7")
for link in "${link_address[@]}"
do
	dpdk-devbind.py --bind=ice $link
done

link_name=("enp161s0f0" "enp161s0f1" "enp161s0f2" "enp161s0f3" "enp161s0f4" "enp161s0f5" "enp161s0f6" "enp161s0f7" )
for link in "${link_name[@]}"
do
    ip link set $link up
done

#if the error "/usr/bin/env: python: No such file or directory" then enter "sudo ln -s /usr/bin/python3 /usr/bin/python" and try again