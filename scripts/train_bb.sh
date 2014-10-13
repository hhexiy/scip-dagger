#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -x <suffix> -p <num_passes> -n <num_per_iter> -c <svm_c> -w <svm_w> -e <experiment> -m <problem>"
}

suffix=".lp.gz"
problem="general"

while getopts ":hd:p:n:c:e:w:tx:m:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      echo "training data: dat/$data"
      ;;
    p)
      numPasses=${OPTARG}
      echo "number of passes: $numPasses"
      ;;
    n)
      numPerIter=${OPTARG}
      echo "number of examples per iteration: $numPerIter"
      ;;
    c)
      svmc=${OPTARG}
      echo "svm parameter c: $svmc"
      ;;
    w)
      svmw=${OPTARG}
      echo "svm parameter w: $svmw"
      ;;
    e)
      experiment=${OPTARG}
      echo "experiment name: $experiment"
      ;;
    x)
      suffix=${OPTARG}
      echo "data suffix: $suffix"
      ;;
    m)
      problem=${OPTARG}
      echo "problem: $problem"
      ;;
    :)
      echo "ERROR: -${OPTARG} requires an argument"
      usage
      exit 1
      ;;
    ?)
      echo "ERROR: unknown option -${OPTARG}"
      usage
      exit 1
      ;;
  esac
done

datDir="dat/$data"
solDir="solution/$data"
scratch="/scratch0"

trjDir=$scratch/hhe/scip-dagger/trj/$data/$experiment
if ! [ -d $trjDir ]; then mkdir -p $trjDir; fi
searchTrj=$trjDir/"search.trj"
killTrj=$trjDir/"kill.trj"
# We need to append to these trj
if [ -e $searchTrj ]; then rm $searchTrj; echo "rm $searchTrj"; fi
if [ -e $killTrj ]; then rm $killTrj; echo "rm $killTrj"; fi
if [ -e $searchTrj.weight ]; then rm $searchTrj.weight; fi
if [ -e $killTrj.weight ]; then rm $killTrj.weight; fi

policyDir=policy/$data/$experiment
if ! [ -d $policyDir ]; then mkdir -p $policyDir; fi
searchPolicy=""
killPolicy=""
numPolicy=0

num=1
for i in `seq 1 $numPasses`; do
  for prob in `ls $datDir | sort -R`; do
    base=`sed "s/$suffix//g" <<< $prob`
    prob=$datDir/$prob
    sol=$solDir/$base.sol
    echo $prob $sol

    killTrjIter=$trjDir/$base.kill.trj.$numPolicy
    searchTrjIter=$trjDir/$base.search.trj.$numPolicy

    if [ -z $searchPolicy ]; then
      # First round, no policy yet
      echo "Gathering first iteration trajectory data"
      bin/scipdagger -f $prob -o $sol --nodesel oracle --nodeseltrj $searchTrjIter --nodepru oracle --nodeprutrj $killTrjIter
      cat $searchTrjIter >> $searchTrj
      cat $searchTrjIter.weight >> $searchTrj.weight
      cat $killTrjIter >> $killTrj
      cat $killTrjIter.weight >> $killTrj.weight
    else
      # Search with policy 
      echo "Gathering trajectory data with $policy"
      bin/scipdagger -f $prob -o $sol --nodesel dagger $searchPolicy --nodeseltrj $searchTrjIter --nodepru dagger $killPolicy --nodeprutrj $killTrjIter
      cat $searchTrjIter >> $searchTrj
      cat $searchTrjIter.weight >> $searchTrj.weight
      cat $killTrjIter >> $killTrj
      cat $killTrjIter.weight >> $killTrj.weight
    fi
    rm $killTrjIter $searchTrjIter $killTrjIter.weight $searchTrjIter.weight

    # Learn a policy after a few examples
    if [ `echo "$num % $numPerIter" | bc` -eq 0 ]; then
      if ! [ -d $scratch/$data/$experiment ]; then mkdir -p $scratch/$data/$experiment; fi

      searchPolicy=$policyDir/searchPolicy.$numPolicy
      echo "c = $svmc/$(avg $searchTrj.weight)"
      c=$(echo "scale=6; $svmc/$(avg $searchTrj.weight)" | bc)
      echo "Training search policy $numPolicy with svm c=$c"
      bin/train-w -c $c -W $searchTrj.weight $searchTrj $searchPolicy
      bin/predict $searchTrj $searchPolicy $scratch/$data/$experiment/pred

      killPolicy=$policyDir/killPolicy.$numPolicy
      echo "c = $svmc/$(avg $killTrj.weight)"
      c=$(echo "scale=6; $svmc/$(avg $killTrj.weight)" | bc)
      if [ $numPolicy == 0 ]; then w=1; else w=$svmw; fi
      echo "Training node kill policy $numPolicy with svm c=$c and w-1=$w"
      bin/train-w -c $c -w-1 $w -W $killTrj.weight $killTrj $killPolicy
      bin/predict $killTrj $killPolicy $scratch/$data/$experiment/pred

      searchPolicy=$policyDir/searchPolicy.$numPolicy
      killPolicy=$policyDir/killPolicy.$numPolicy
      numPolicy=$((numPolicy+1)) 
    fi
    num=$((num+1)) 
  done
done

rm $trjDir/*

