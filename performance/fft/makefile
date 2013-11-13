CFLAGS = -O0 -g 

train:
	m4 c.m4.null.pthread fft.C > fft.c
	
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang $(CFLAGS) -emit-llvm -c fft.c -o fft.bc

	# AIPrepare
	$(AI_HOME)/build/tools/id-server fft.AccessInfo &
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAIPrepare.so -AIPrepare <fft.bc >fft-Marked.bc
	killall id-server

	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITrace.so -AITrace <fft-Marked.bc >fft-Trace.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc fft-Trace.bc -o fft-Trace.s

	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang fft-Trace.s -o fft.trace -lai -lpthread -lm

	# Training
	python run.py train 4 ./fft
	# Compute Bsets
	$(AI_HOME)/build/tools/trace2bset 4 trace fft.AccessInfo fft.BSet
	# Sanitize Bsets
	$(AI_HOME)/build/tools/sanitize-bset fft.BSet

train2: sanitized-bset.bin sanitized-bset.txt fft-Marked.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITrace.so -AITrace -AIBSet=fft.BSet <fft-Marked.bc >fft-Trace2.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc fft-Trace2.bc -o fft-Trace2.s

	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang fft-Trace2.s -o fft.trace2 -lai -lpthread -lm

	# Training
	python run.py train2 20 ./fft
	# Compute Bsets
	$(AI_HOME)/build/tools/trace2bset 20 trace2 fft.AccessInfo fft.BSet
	# Sanitize Bsets
	$(AI_HOME)/build/tools/sanitize-bset fft.BSet

	make relax
	python run.py relax 100 ./fft
	

relax: sanitized-bset.bin sanitized-bset.txt fft-Marked.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITolerate.so -AITolerate -AISanitizedBSetFile=sanitized-bset.txt -AIRelax=1 <fft-Marked.bc >fft-Relax.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc fft-Relax.bc -o fft-Relax.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang fft-Relax.s -o fft.relax -lai -lpthread -lm

test: sanitized-bset.bin sanitized-bset.txt fft-Marked.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc fft-Marked.bc -o fft.s
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITolerate.so -AITolerate -AISanitizedBSetFile=sanitized-bset.txt <fft-Marked.bc >fft-Tolerate.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc fft-Tolerate.bc -o fft-Tolerate.s

	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang fft.s -o fft.native -lpthread -lm
	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang fft-Tolerate.s -o fft.instrumented -lai -lpthread -lm

bias: fft.group
	$(AI_ARCHIVE_DIR)/llvm/install/bin/opt -load=$(AI_HOME)/build/llvm/AI/LLVMAITolerate.so -AITolerate -AISanitizedBSetFile=sanitized-bset.txt -AIInsGroupFile=fft.group -AIInsGroupThreshold=0.3 <fft-Marked.bc >fft-Bias.bc
	$(AI_ARCHIVE_DIR)/llvm/install/bin/llc fft-Bias.bc -o fft-Bias.s

	$(AI_ARCHIVE_DIR)/llvm/install/bin/clang fft-Bias.s -o fft.bias -lai -lpthread -lm

clean:
	rm -f *.bc *.s *.AccessInfo *.bin trace.* *.trace *.BSet *.native *.instrumented *.txt *.c *.h *.tmp
