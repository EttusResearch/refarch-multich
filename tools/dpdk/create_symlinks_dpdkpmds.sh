mkdir -p /usr/local/lib/dpdk-pmds 

for i in /usr/local/lib/librte_mempool_*
do
ln -s "$i" /usr/local/lib/dpdk-pmds/"${i##*/}"
done

for i in /usr/local/lib/librte_pmd_*
do
ln -s "$i" /usr/local/lib/dpdk-pmds/"${i##*/}"
done