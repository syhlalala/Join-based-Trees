#bash
clear
/bin/cp /dev/null results.txt


CPU_COUNT=$(nproc)

declare -a tCount=( $(for ((t = 1; t <= CPU_COUNT; t*=2)) do echo $t; done) )
if [ ${tCount[-1]} -ne $CPU_COUNT ]
then
    tCount+=($CPU_COUNT);
fi

UNION=1
INTERSECT=2
INSERT=3
BUILD=4
FILTER=5
DE_UNION=6
DE_INTERSECT=7
SPLIT=8
DIFFERENCE=9
SEQ_UNION=10

REPEAT=3

LOW=4
HIGH=8

let N=$((10**$LOW));
let M=$((10**$LOW));

for ((t1 = LOW; t1 <= HIGH; t1++))
do
    let M=$((10**$LOW));
    for ((t2 = LOW; t2 <= t1; t2++))
    do
        echo "Union Test:" | tee -a results.txt
        echo "N = $N" | tee -a results.txt;
        echo "M = $M" | tee -a results.txt;
        for threads in "${tCount[@]}"
        do
            echo "... on '$threads' threads" | tee -a results.txt
            export CILK_NWORKERS="$threads";
            ./testParallel "$UNION" "$N" "$M" "$REPEAT" >> results.txt
        done
        echo "" | tee -a results.txt
        let M=$((M*10));
    done
    let N=$((N*10));
done

let N=$((10**$HIGH));
let M=$((10**$LOW));
export CILK_NWORKERS="1";

for ((t1 = LOW; t1 <= HIGH; t1++))
do
    echo "Sequential Union test (N = $N):" | tee -a results.txt
    echo "M = $M" | tee -a results.txt;
    ./testParallel "$SEQ_UNION" "$N" "$M" "$REPEAT" >> results.txt
    M=$((M*10));
done

echo "" | tee -a results.txt

let N=$((10**$LOW));
let M=$((10**$LOW));

for ((t1 = LOW; t1 <= HIGH; t1++))
do
    let M=$((10**$LOW));
    for ((t2 = LOW; t2 <= t1; t2++))
    do
        echo "Destructive Union Test:" | tee -a results.txt
        echo "N = $N" | tee -a results.txt;
        echo "M = $M" | tee -a results.txt;
        for threads in "${tCount[@]}"
        do
            echo "... on '$threads' threads" | tee -a results.txt
            export CILK_NWORKERS="$threads";
            ./testParallel "$DE_UNION" "$N" "$M" "$REPEAT" >> results.txt
        done
        echo "" | tee -a results.txt
        let M=$((M*10));
    done
    let N=$((N*10));
done

let N=$((10**$LOW));
let M=$((10**$LOW));

for ((t1 = LOW; t1 <= HIGH; t1++))
do
    let M=$((10**$LOW));
    for ((t2 = LOW; t2 <= t1; t2++))
    do
        echo "Intersect Test:" | tee -a results.txt
        echo "N = $N" | tee -a results.txt;
        echo "M = $M" | tee -a results.txt;
        for threads in "${tCount[@]}"
        do
            echo "... on '$threads' threads" | tee -a results.txt
            export CILK_NWORKERS="$threads";
            ./testParallel "$INTERSECT" "$N" "$M" "$REPEAT" >> results.txt
        done
        echo "" | tee -a results.txt
        let M=$((M*10));
    done
    let N=$((N*10));
done

let N=$((10**$LOW));
let M=$((10**$LOW));

for ((t1 = LOW; t1 <= HIGH; t1++))
do
    let M=$((10**$LOW));
    for ((t2 = LOW; t2 <= t1; t2++))
    do
        echo "Destructive Intersect Test:" | tee -a results.txt
        echo "N = $N" | tee -a results.txt;
        echo "M = $M" | tee -a results.txt;
        for threads in "${tCount[@]}"
        do
            echo "... on '$threads' threads" | tee -a results.txt
            export CILK_NWORKERS="$threads";
            ./testParallel "$DE_INTERSECT" "$N" "$M" "$REPEAT" >> results.txt
        done
        echo "" | tee -a results.txt
        let M=$((M*10));
    done
    let N=$((N*10));
done

let N=$((10**$LOW));
let M=$((10**$LOW));

for ((t1 = LOW; t1 <= HIGH; t1++))
do
    let M=$((10**$LOW));
    for ((t2 = LOW; t2 <= t1; t2++))
    do
        echo "Difference Test:" | tee -a results.txt
        echo "N = $N" | tee -a results.txt;
        echo "M = $M" | tee -a results.txt;
        for threads in "${tCount[@]}"
        do
            echo "... on '$threads' threads" | tee -a results.txt
            export CILK_NWORKERS="$threads";
            ./testParallel "$DIFFERENCE" "$N" "$M" "$REPEAT" >> results.txt
        done
        echo "" | tee -a results.txt
        let M=$((M*10));
    done
    let N=$((N*10));
done

let N=$((10**$LOW));
export CILK_NWORKERS="1";
for ((t = LOW; t <= HIGH; t++))
do
    echo "Insertion test:" | tee -a results.txt
    echo "N = $N" | tee -a results.txt;
    ./testParallel "$INSERT" "$N" "0" "$REPEAT" >> results.txt
    let N=$((N*10));
done

echo "" | tee -a results.txt

let N=$((10**$LOW));
for ((t = LOW; t <= HIGH; t++))
do
    echo "Filter test:" | tee -a results.txt
    echo "N = $N" | tee -a results.txt;
    for threads in "${tCount[@]}"
    do
       echo "... on '$threads' threads" | tee -a results.txt
       export CILK_NWORKERS="$threads";
       ./testParallel "$FILTER" "$N" "0" "$REPEAT" >> results.txt
    done
    let N=$((N*10));
done

echo "" | tee -a results.txt

let N=$((10**$LOW));
for ((t = LOW; t <= HIGH; t++))
do
    echo "Build test:" | tee -a results.txt
    echo "N = $N" | tee -a results.txt;
    for threads in "${tCount[@]}"
    do
        echo "... on '$threads' threads" | tee -a results.txt
        export CILK_NWORKERS="$threads";
        ./testParallel "$BUILD" "$N" "0" "$REPEAT" >> results.txt
    done
    let N=$((N*10));
done

echo "" | tee -a results.txt

if false
then

let N=$((10**$LOW));
export CILK_NWORKERS="1";
for ((t = LOW; t <= HIGH; t++))
do
    echo "Split test:" | tee -a results.txt
    echo "N = $N" | tee -a results.txt;
    ./testParallel "$SPLIT" "$N" "0" "$REPEAT" >> results.txt
    let N=$((N*10));
done

fi
