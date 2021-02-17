#!/bin/bash

NC='\033[0;0m'
BLACK='\033[0;30m'
DGRAY='\033[1;30m'
RED='\033[0;31m'
LRED='\033[1;31m'
GREEN='\033[0;32m'
LGREEN='\033[1;32m'
BROWN='\033[0;33m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
LBLUE='\033[1;34m'
PURPLE='\033[0;35m'
LPURPLE='\033[1;35m'
CYAN='\033[0;36m'
LCYAN='\033[1;36m'
LGRAY='\033[0;37m'
WHITE='\033[1;37m'

tests=`find ./tests -name "*.test.out"`
tests=($tests)
success=0

echo -n "" > ./tests.log

#echo tests names: $tests
count=${#tests[@]}
echo
echo -e Running ${YELLOW}$count ${NC}tests
echo

for ((i=0; i<$count; i++))
do
   testname=${tests[i]}
   echo -e -n ${CYAN}$testname'\t'${YELLOW}$((i+1))/$count'\t'${NC}
   $testname >> ./tests.log
   statu=$?
   if [ $statu == 0 ]
   then
      echo -e ${GREEN}Success${NC}
      ((success=success+1))
   else
      echo -e ${RED}Failure ${NC}$statu
   fi
done

echo
if [ $count == $success ]
then
   echo -e ${GREEN}All tests passed!${NC}
else
   echo -e ${YELLOW}$success ${NC}out of ${YELLOW}$count ${NC}succeeded
fi
echo
