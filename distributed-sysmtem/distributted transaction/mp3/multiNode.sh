#!/bin/bash

PASSWORDS="Adams7203311!"





for i in $(seq 1 9)
do
  sshpass -p ${PASSWORDS} scp -o StrictHostKeyChecking=no -rp client coordinator server *.sh qw2@sp20-cs425-g02-0$i.cs.illinois.edu:~/mp3
done

sshpass -p ${PASSWORDS} scp -o StrictHostKeyChecking=no -rp client coordinator server *.sh qw2@sp20-cs425-g02-10.cs.illinois.edu:~/mp3
: <<'END'
for i in $(seq 1 9)
do
  sshpass -p ${PASSWORDS} scp -o StrictHostKeyChecking=no -p qw2@sp20-cs425-g02-0$i.cs.illinois.edu:~/mp3/*.txt ~/go/src/ece428/mp3
done
  sshpass -p ${PASSWORDS} scp -o StrictHostKeyChecking=no -p qw2@sp20-cs425-g02-10.cs.illinois.edu:~/mp3/*.txt ~/go/src/ece428/mp3
END
