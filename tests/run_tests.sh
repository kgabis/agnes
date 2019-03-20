#!/bin/bash

# set -xe

trap "exit" INT

if [ "$#" -ne 1 ]; then
    echo "Usage: run_tests.sh ROM_DIRECTORY"
	exit 1
fi

ROM_DIR=$1

TESTS_OK=true

printf "Compiling player\n"
make player
if [ ${?} != "0" ]; then
	echo " FAIL"
	TESTS_OK=false
else
	echo "	OK"
	PLAYER_COMPILED=true
fi

printf "\nCompiling recorder\n"
make recorder
if [ ${?} != "0" ]; then
	echo " FAIL"
	TESTS_OK=false
else
	echo "	OK"
fi

printf "\nCompiling examples\n"
pushd ../examples
make
if [ ${?} != "0" ]; then
	echo " FAIL"
	TESTS_OK=false
else
	echo "	OK"
fi
make clean
popd

printf "\nPlaying games\n"
if [ "${PLAYER_COMPILED}" == true ]; then
	echo "Unpacking recordings"
	tar -zxvf recs.tar.gz
	if [ ${?} != "0" ]; then
		echo " FAIL"
		TESTS_OK=false
	else
		echo "	OK"
	fi

	START=$(date +%s)

	for f in recs/*.json; do 
		./player --recordings "$f" --roms-dir "${ROM_DIR}" --mode verify --no-vsync --frame-skip 10 --no-render
		if [ ${?} != "0" ]; then
			TESTS_OK=false
		fi
	done

	END=$(date +%s)
	DIFF=$(echo "$END - $START" | bc)
	echo "Time: ${DIFF} seconds"

	rm -rf recs
fi

make clean

echo ""

if [ "${TESTS_OK}" == true ]; then
	echo "ALL TESTS SUCCEEDED"
else
	echo "TESTS FAILED"
fi

