link_name=("enp225s0f0" "enp225s0f1" "enp225s0f2" "enp225s0f3" "enp193s0f0" "enp193s0f1" "enp193s0f2" "enp193s0f3" "enp1s0f0" "enp1s0f1" "enp1s0f2" "enp1s0f3" "enp37s0f0" "enp37s0f1" "enp37s0f2" "enp37s0f3")
for link in "${link_name[@]}"
do
    sudo ethtool -G $link tx 4096 rx 4096
done