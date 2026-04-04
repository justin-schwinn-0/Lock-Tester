
FILE_LOC="node-cmd-files"


rm ${FILE_LOC} -rf
mkdir ${FILE_LOC}
python3 multiTester.py --threads 128 --trials 5 --seconds 5 --numNodes 16 --nodeName milan --outDir ${FILE_LOC}
python3 multiTester.py --threads 112 --trials 5 --seconds 5 --numNodes 14 --nodeName spr --outDir ${FILE_LOC}
python3 multiTester.py --threads 80 --trials 5 --seconds 5 --numNodes 10 --nodeName icx --outDir ${FILE_LOC}
python3 multiTester.py --threads 48 --trials 5 --seconds 5 --numNodes 7 --nodeName skx --outDir ${FILE_LOC}
