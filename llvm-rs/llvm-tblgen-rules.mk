###########################################################
## Commands for running tblgen to compile a td file
##########################################################
define transform-td-to-out
$(if $(LOCAL_IS_HOST_MODULE),	\
	$(call transform-host-td-to-out,$(1)),	\
	$(call transform-device-td-to-out,$(1)))
endef

###########################################################
## TableGen: Compile .td files to .inc.
###########################################################

# Set LOCAL_MODULE_CLASS to STATIC_LIBRARIES default (require
# for macro local-intermediates-dir)
ifeq ($(LOCAL_MODULE_CLASS),)
	LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

ifneq ($(strip $(TBLGEN_TABLES)),)

intermediates := $(call local-intermediates-dir)
tblgen_gen_tables := $(addprefix $(intermediates)/,$(TBLGEN_TABLES))
LOCAL_GENERATED_SOURCES += $(tblgen_gen_tables)

tblgen_source_dir := $(LOCAL_PATH)
ifneq ($(TBLGEN_TD_DIR),)
tblgen_source_dir := $(TBLGEN_TD_DIR)
endif

#
# The directory and the .td directory is not the same.
#
ifeq ($(tblgen_source_dir),$(LLVM_ROOT_PATH)/lib/Target/ARM/MCTargetDesc)
$(intermediates)/%GenRegisterInfo.inc: $(tblgen_source_dir)/../%.td $(TBLGEN)
	$(call transform-td-to-out, register-info)
$(intermediates)/%GenInstrInfo.inc: $(tblgen_source_dir)/../%.td $(TBLGEN)
	$(call transform-td-to-out,instr-info)
$(intermediates)/%GenSubtargetInfo.inc: $(tblgen_source_dir)/../%.td $(TBLGEN)
	$(call transform-td-to-out,subtarget)
$(intermediates)/ARMGenMCCodeEmitter.inc: $(tblgen_source_dir)/../ARM.td $(TBLGEN)
	$(call transform-td-to-out,emitter -mc-emitter)
endif

ifeq ($(tblgen_source_dir),$(LLVM_ROOT_PATH)/lib/Target/X86/MCTargetDesc)
$(intermediates)/%GenRegisterInfo.inc: $(tblgen_source_dir)/../%.td $(TBLGEN)
	$(call transform-td-to-out, register-info)
$(intermediates)/%GenInstrInfo.inc: $(tblgen_source_dir)/../%.td $(TBLGEN)
	$(call transform-td-to-out,instr-info)
$(intermediates)/%GenSubtargetInfo.inc: $(tblgen_source_dir)/../%.td $(TBLGEN)
	$(call transform-td-to-out,subtarget)
endif


ifneq ($(filter %GenRegisterInfo.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenRegisterInfo.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,register-info)
endif

ifneq ($(filter %GenInstrInfo.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenInstrInfo.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,instr-info)
endif

ifneq ($(filter %GenAsmWriter.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenAsmWriter.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,asm-writer)
endif

ifneq ($(filter %GenAsmWriter1.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenAsmWriter1.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,asm-writer -asmwriternum=1)
endif

ifneq ($(filter %GenAsmMatcher.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenAsmMatcher.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,asm-matcher)
endif

ifneq ($(filter %GenCodeEmitter.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenCodeEmitter.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,emitter)
endif

ifneq ($(filter %GenMCCodeEmitter.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenMCCodeEmitter.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,emitter -mc-emitter)
endif

ifneq ($(filter %GenMCPseudoLowering.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenMCPseudoLowering.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,pseudo-lowering)
endif

ifneq ($(filter %GenDAGISel.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenDAGISel.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,dag-isel)
endif

ifneq ($(filter %GenDisassemblerTables.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenDisassemblerTables.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,disassembler)
endif

ifneq ($(filter %GenEDInfo.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenEDInfo.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,enhanced-disassembly-info)
endif

ifneq ($(filter %GenFastISel.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenFastISel.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,fast-isel)
endif

ifneq ($(filter %GenSubtargetInfo.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenSubtargetInfo.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,subtarget)
endif

ifneq ($(filter %GenCallingConv.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenCallingConv.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,callingconv)
endif

ifneq ($(filter %GenIntrinsics.inc,$(tblgen_gen_tables)),)
$(intermediates)/%GenIntrinsics.inc: $(tblgen_source_dir)/%.td $(TBLGEN)
	$(call transform-td-to-out,tgt_intrinsics)
endif

ifneq ($(findstring ARMGenDecoderTables.inc,$(tblgen_gen_tables)),)
$(intermediates)/ARMGenDecoderTables.inc: $(tblgen_source_dir)/ARM.td $(TBLGEN)
	$(call transform-td-to-out,arm-decoder)
endif
ifneq ($(findstring ARMGenDisassemblerTables.inc,$(tblgen_gen_tables)),)
$(intermediates)/ARMGenDisassemblerTables.inc: $(tblgen_source_dir)/ARM.td $(TBLGEN)
	$(call transform-td-to-out,disassembler)
endif
endif
