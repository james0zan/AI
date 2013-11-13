train:
	# Convert the C++ code to LLVM IR
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ -O0 -g -I$(AI_HOME)/ai-api -emit-llvm -c test.cc -o test.bc

	# Start the `id-server`
	$(AI_HOME)/build/tools/id-server test.AccessInfo &
	# Perform the AIPrepare pass, with the "-WhiteListFileName" flag
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAIPrepare.so -AIPrepare -WhiteListFileName="whitelist.ins" <test.bc >test-Marked.bc
	# Stop the `id-server`
	killall id-server

	# Perform the AITrace pass
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITrace.so -AITrace <test-Marked.bc >test-Trace.bc
	# Compile to the executable
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc test-Trace.bc -o test-Trace.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang++ test-Trace.s -o test.trace -lai -lpthread

	# Training
	bash ./train.sh 10 ./test.trace
	# Compute Bsets
	$(AI_HOME)/build/tools/trace2bset 10 trace test.AccessInfo test.BSet
	# Sanitize Bsets
	$(AI_HOME)/build/tools/sanitize-bset test.BSet
	
	
custom: test.custom.BSet
	# Sanitize the Custom Bsets
	$(AI_HOME)/build/tools/sanitize-bset test.custom.BSet
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITolerate.so -AITolerate -AISanitizedBSetFile=sanitized-bset.txt <test-Marked.bc >test-Custom.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llvm-dis <test-Custom.bc >test-Custom.ll

clean:
	rm -f *.bc *.s *.AccessInfo *.bin trace.* *.trace *.BSet *.native *.instrumented *.txt *.c *.h *.tmp whitelist.* *.ll *.group