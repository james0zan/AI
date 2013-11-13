train: pbzip2.cpp
	# Convert the C++ code to LLVM IR
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ -O0 -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -emit-llvm -c pbzip2.cpp -o pbzip2.bc

	# Start the `id-server`
	$(AI_HOME)/build/tools/id-server pbzip2.AccessInfo &
	# Perform the AIPrepare pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAIPrepare.so -AIPrepare <pbzip2.bc >pbzip2-Marked.bc
	# Stop the `id-server`
	killall id-server

	# Perform the AITrace pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITrace.so -AITrace <pbzip2-Marked.bc >pbzip2-Trace.bc
	# Compile to the executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc pbzip2-Trace.bc -o pbzip2-Trace.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ pbzip2-Trace.s -o pbzip2.trace -lai -lpthread -lbz2

	# Training
	bash ./train.sh 50 ./pbzip2.trace
	# Compute Bsets
	$(AI_HOME)/build/tools/trace2bset 50 trace pbzip2.AccessInfo pbzip2.BSet
	# Sanitize Bsets
	$(AI_HOME)/build/tools/sanitize-bset pbzip2.BSet

tolerated: pbzip2.cpp sanitized-bset.bin sanitized-bset.txt
	# Convert the TRIGGERED version of C++ code to LLVM IR
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ -O0 -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DTRIGGERTHEBUG -emit-llvm -c pbzip2.cpp -o pbzip2-triggered.bc

	# Build the triggered executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc pbzip2-triggered.bc -o pbzip2-triggered.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ pbzip2-triggered.s -o pbzip2.triggered -lpthread -lbz2

	# Start the `id-server` again
	$(AI_HOME)/build/tools/id-server pbzip2-triggered.AccessInfo &
	# Perform the AIPrepare pass
	# Since only the value of global variables are altered,
	# the AIPrepare pass is assumed to assign the same ID to these two version of code.
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAIPrepare.so -AIPrepare <pbzip2-triggered.bc >pbzip2-triggered-Marked.bc
	# Stop the `id-server`
	killall id-server

	# Perform the AITolerate pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITolerate.so -AITolerate -AISanitizedBSetFile=sanitized-bset.txt <pbzip2-triggered-Marked.bc >pbzip2-Tolerate.bc
	# Compile to the executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc pbzip2-Tolerate.bc -o pbzip2-Tolerate.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ pbzip2-Tolerate.s -o pbzip2.tolerated -lai -lpthread -lbz2



clean:
	rm -f *.bc *.s *.AccessInfo *.bin trace.* *.trace *.BSet *.triggered *.tolerated *.txt *.bz2 *.mem
