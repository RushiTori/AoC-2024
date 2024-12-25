challenges=("dummy" "HistorianHysteria" "RedNosedReports" "MullItOver" "CeresSearch" "PrintQueue" "GuardGallivant" "BridgeRepair" "ResonantCollinearity" "DiskFragmenter" "HoofIt" "PlutonianPebbles" "GardenGroups" "ClawContraption" "RestroomRedoubt" "WarehouseWoes" "ReindeerMaze" "ChronospatialComputer" "RAMRun" "LinenLayout" "RaceCondition" "KeypadConundrum" "MonkeyMarket" "LANParty" "CrossedWires" "CodeChronicle")

for i in $(seq 01 25); do
	printf -v j "%02d" $i
	cd ./$j/
	make
	cd ../
done

for i in $(seq 01 25); do
	printf -v j "%02d" $i
	cd ./$j/
	echo ${challenges[i]} ":"
	./${challenges[i]}.out
	echo " "
	cd ../
done
