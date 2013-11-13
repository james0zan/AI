train: example.cc
	# Convert the C++ code to LLVM IR
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ -O0 -g -emit-llvm -c example.cc -o example.bc

	# Start the `id-server`
	$(AI_HOME)/build/tools/id-server ex.AccessInfo &
	# Perform the AIPrepare pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAIPrepare.so -AIPrepare <example.bc >example-Marked.bc
	# Stop the `id-server`
	killall id-server

	# Perform the AITrace pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITrace.so -AITrace <example-Marked.bc >example-Trace.bc
	# Compile to the executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc example-Trace.bc -o example-Trace.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ example-Trace.s -o example.trace -lai -lpthread

	# Training
	bash ./train.sh 10 ./example.trace
	# Compute Bsets
	$(AI_HOME)/build/tools/trace2bset 10 trace ex.AccessInfo ex.BSet
	# Sanitize Bsets
	$(AI_HOME)/build/tools/sanitize-bset ex.BSet

tolerated: example.cc sanitized-bset.bin sanitized-bset.txt
	# Convert the TRIGGERED version of C++ code to LLVM IR
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ -DTRIGGERTHEBUG -g -emit-llvm -c example.cc -o example-triggered.bc

	# Build the triggered executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc example-triggered.bc -o example-triggered.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ example-triggered.s -o example.triggered -lpthread

	# Start the `id-server` again
	$(AI_HOME)/build/tools/id-server ex-triggered.AccessInfo &
	# Perform the AIPrepare pass
	# Since only the value of global variables are altered,
	# the AIPrepare pass is assumed to assign the same ID to these two version of code.
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAIPrepare.so -AIPrepare <example-triggered.bc >example-triggered-Marked.bc
	# Stop the `id-server`
	killall id-server

	# Perform the AITolerate pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITolerate.so -AITolerate -AISanitizedBSetFile=sanitized-bset.txt <example-triggered-Marked.bc >example-Tolerate.bc
	# Compile to the executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc example-Tolerate.bc -o example-Tolerate.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ example-Tolerate.s -o example.tolerated -lai -lpthread


clean:
	rm -f *.bc *.s *.AccessInfo *.bin trace.* *.trace *.BSet *.triggered *.tolerated *.txt *.mem
