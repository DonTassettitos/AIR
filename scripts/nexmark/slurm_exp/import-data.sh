#!/bin/bash
DATE=$( date +"%d%m%Y" )
DIRECTORY=observations-$DATE

rm -rf $DIRECTORY
rsync HPC:scripts/nexmark/slurm_exp/observations/* $DIRECTORY
