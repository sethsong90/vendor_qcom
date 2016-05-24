//===-- ARMMCTargetDesc.cpp - ARM Target Descriptions ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides ARM specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "ARMMCTargetDesc.h"
#include "ARMMCAsmInfo.h"
#include "ARMBaseInfo.h"
#include "InstPrinter/ARMInstPrinter.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_REGINFO_MC_DESC
#include "ARMGenRegisterInfo.inc"

#define GET_INSTRINFO_MC_DESC
#include "ARMGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "ARMGenSubtargetInfo.inc"

using namespace llvm;

std::string ARM_MC::ParseARMTriple(StringRef TT) {
  // Set the boolean corresponding to the current target triple, or the default
  // if one cannot be determined, to true.
  unsigned Len = TT.size();
  unsigned Idx = 0;

  // FIXME: Enhance Triple helper class to extract ARM version.
  bool isThumb = false;
  if (Len >= 5 && TT.substr(0, 4) == "armv")
    Idx = 4;
  else if (Len >= 6 && TT.substr(0, 5) == "thumb") {
    isThumb = true;
    if (Len >= 7 && TT[5] == 'v')
      Idx = 6;
  }

  std::string ARMArchFeature;
  if (Idx) {
    unsigned SubVer = TT[Idx];
    if (SubVer >= '7' && SubVer <= '9') {
      if (Len >= Idx+2 && TT[Idx+1] == 'm') {
        // v7m: FeatureNoARM, FeatureDB, FeatureHWDiv, FeatureMClass
        ARMArchFeature = "+v7,+noarm,+db,+hwdiv,+mclass";
      } else if (Len >= Idx+3 && TT[Idx+1] == 'e'&& TT[Idx+2] == 'm') {
        // v7em: FeatureNoARM, FeatureDB, FeatureHWDiv, FeatureDSPThumb2,
        //       FeatureT2XtPk, FeatureMClass
        ARMArchFeature = "+v7,+noarm,+db,+hwdiv,+t2dsp,t2xtpk,+mclass";
      } else
        // v7a: FeatureNEON, FeatureDB, FeatureDSPThumb2, FeatureT2XtPk
        ARMArchFeature = "+v7,+neon,+db,+t2dsp,+t2xtpk";
    } else if (SubVer == '6') {
      if (Len >= Idx+3 && TT[Idx+1] == 't' && TT[Idx+2] == '2')
        ARMArchFeature = "+v6t2";
      else if (Len >= Idx+2 && TT[Idx+1] == 'm')
        // v6m: FeatureNoARM, FeatureMClass
        ARMArchFeature = "+v6t2,+noarm,+mclass";
      else
        ARMArchFeature = "+v6";
    } else if (SubVer == '5') {
      if (Len >= Idx+3 && TT[Idx+1] == 't' && TT[Idx+2] == 'e')
        ARMArchFeature = "+v5te";
      else
        ARMArchFeature = "+v5t";
    } else if (SubVer == '4' && Len >= Idx+2 && TT[Idx+1] == 't')
      ARMArchFeature = "+v4t";
  }

  if (isThumb) {
    if (ARMArchFeature.empty())
      ARMArchFeature = "+thumb-mode";
    else
      ARMArchFeature += ",+thumb-mode";
  }

  return ARMArchFeature;
}

MCSubtargetInfo *ARM_MC::createARMMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                  StringRef FS) {
  std::string ArchFS = ARM_MC::ParseARMTriple(TT);
  if (!FS.empty()) {
    if (!ArchFS.empty())
      ArchFS = ArchFS + "," + FS.str();
    else
      ArchFS = FS;
  }

  MCSubtargetInfo *X = new MCSubtargetInfo();
  InitARMMCSubtargetInfo(X, TT, CPU, ArchFS);
  return X;
}

static MCInstrInfo *createARMMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitARMMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createARMMCRegisterInfo(StringRef Triple) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitARMMCRegisterInfo(X, ARM::LR);
  return X;
}

static MCAsmInfo *createARMMCAsmInfo(const Target &T, StringRef TT) {
  Triple TheTriple(TT);

  if (TheTriple.isOSDarwin())
    return new ARMMCAsmInfoDarwin();

  return new ARMELFMCAsmInfo();
}

static MCCodeGenInfo *createARMMCCodeGenInfo(StringRef TT, Reloc::Model RM,
                                             CodeModel::Model CM,
                                             CodeGenOpt::Level OL) {
  MCCodeGenInfo *X = new MCCodeGenInfo();
  if (RM == Reloc::Default) {
    Triple TheTriple(TT);
    // Default relocation model on Darwin is PIC, not DynamicNoPIC.
    RM = TheTriple.isOSDarwin() ? Reloc::PIC_ : Reloc::DynamicNoPIC;
  }
  X->InitMCCodeGenInfo(RM, CM, OL);
  return X;
}

// This is duplicated code. Refactor this.
static MCStreamer *createMCStreamer(const Target &T, StringRef TT,
                                    MCContext &Ctx, MCAsmBackend &MAB,
                                    raw_ostream &OS,
                                    MCCodeEmitter *Emitter,
                                    bool RelaxAll,
                                    bool NoExecStack) {
  Triple TheTriple(TT);

  if (TheTriple.isOSDarwin())
    return createMachOStreamer(Ctx, MAB, OS, Emitter, false);

  if (TheTriple.isOSWindows()) {
    llvm_unreachable("ARM does not support Windows COFF format");
  }

  return createELFStreamer(Ctx, MAB, OS, Emitter, false, NoExecStack);
}

static MCInstPrinter *createARMMCInstPrinter(const Target &T,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI,
                                             const MCSubtargetInfo &STI) {
  if (SyntaxVariant == 0)
    return new ARMInstPrinter(MAI, MII, MRI, STI);
  return 0;
}

namespace {

class ARMMCInstrAnalysis : public MCInstrAnalysis {
public:
  ARMMCInstrAnalysis(const MCInstrInfo *Info) : MCInstrAnalysis(Info) {}

  virtual bool isUnconditionalBranch(const MCInst &Inst) const {
    // BCCs with the "always" predicate are unconditional branches.
    if (Inst.getOpcode() == ARM::Bcc && Inst.getOperand(1).getImm()==ARMCC::AL)
      return true;
    return MCInstrAnalysis::isUnconditionalBranch(Inst);
  }

  virtual bool isConditionalBranch(const MCInst &Inst) const {
    // BCCs with the "always" predicate are unconditional branches.
    if (Inst.getOpcode() == ARM::Bcc && Inst.getOperand(1).getImm()==ARMCC::AL)
      return false;
    return MCInstrAnalysis::isConditionalBranch(Inst);
  }

  uint64_t evaluateBranch(const MCInst &Inst, uint64_t Addr,
                          uint64_t Size) const {
    // We only handle PCRel branches for now.
    if (Info->get(Inst.getOpcode()).OpInfo[0].OperandType!=MCOI::OPERAND_PCREL)
      return -1ULL;

    int64_t Imm = Inst.getOperand(0).getImm();
    // FIXME: This is not right for thumb.
    return Addr+Imm+8; // In ARM mode the PC is always off by 8 bytes.
  }
};

}

static MCInstrAnalysis *createARMMCInstrAnalysis(const MCInstrInfo *Info) {
  return new ARMMCInstrAnalysis(Info);
}

// Force static initialization.
extern "C" void LLVMInitializeARMTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn A(TheARMTarget, createARMMCAsmInfo);
  RegisterMCAsmInfoFn B(TheThumbTarget, createARMMCAsmInfo);

  // Register the MC codegen info.
  TargetRegistry::RegisterMCCodeGenInfo(TheARMTarget, createARMMCCodeGenInfo);
  TargetRegistry::RegisterMCCodeGenInfo(TheThumbTarget, createARMMCCodeGenInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheARMTarget, createARMMCInstrInfo);
  TargetRegistry::RegisterMCInstrInfo(TheThumbTarget, createARMMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheARMTarget, createARMMCRegisterInfo);
  TargetRegistry::RegisterMCRegInfo(TheThumbTarget, createARMMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheARMTarget,
                                          ARM_MC::createARMMCSubtargetInfo);
  TargetRegistry::RegisterMCSubtargetInfo(TheThumbTarget,
                                          ARM_MC::createARMMCSubtargetInfo);

  // Register the MC instruction analyzer.
  TargetRegistry::RegisterMCInstrAnalysis(TheARMTarget,
                                          createARMMCInstrAnalysis);
  TargetRegistry::RegisterMCInstrAnalysis(TheThumbTarget,
                                          createARMMCInstrAnalysis);

  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(TheARMTarget, createARMMCCodeEmitter);
  TargetRegistry::RegisterMCCodeEmitter(TheThumbTarget, createARMMCCodeEmitter);

  // Register the asm backend.
  TargetRegistry::RegisterMCAsmBackend(TheARMTarget, createARMAsmBackend);
  TargetRegistry::RegisterMCAsmBackend(TheThumbTarget, createARMAsmBackend);

  // Register the object streamer.
  TargetRegistry::RegisterMCObjectStreamer(TheARMTarget, createMCStreamer);
  TargetRegistry::RegisterMCObjectStreamer(TheThumbTarget, createMCStreamer);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(TheARMTarget, createARMMCInstPrinter);
  TargetRegistry::RegisterMCInstPrinter(TheThumbTarget, createARMMCInstPrinter);
}

// Instruction pipeline affinity for Scorpion processor.
ARM_MC::PipeType ARM_MC::getScorpionPipeType(unsigned Units)
{
  // Scorpion Instruction Pipeline affinity
  // S pipe (integer load/store): uses LSUnit + AGU
  // VS pipe (Neon/VFP mem): uses LSUnit +  VS
  // VS pipe (Neon/VFP non-mem): uses VS only
  // X pipe (integer ALU): uses ALUX
  // X_Y pipe (simple integer ALU): uses ALUX or ALUY
  // VX pipe: uses VX
  // Branch: is X or S pipe, uses BrUnit

   // Return pipe unknown for empty itineraries.
   if (0 == Units)
     return ARM_MC::PipeType_Unknown;

  if (ScorpionItinerariesFU::SCRPN_LSUnit & Units){
    // can be S or VS
    if (ScorpionItinerariesFU::SCRPN_VSUnit & Units)
      return ARM_MC::PipeType_Scorpion_VS;
    return ARM_MC::PipeType_Scorpion_S;
  }

  if (ScorpionItinerariesFU::SCRPN_VSUnit & Units)
    return ARM_MC::PipeType_Scorpion_VS; // VS non-mem

  if (ScorpionItinerariesFU::SCRPN_ALUX & Units){
    // can be X or X_Y
    if (ScorpionItinerariesFU::SCRPN_ALUY & Units)
      return ARM_MC::PipeType_Scorpion_X_Y;
    return ARM_MC::PipeType_Scorpion_X;
  }

  if (ScorpionItinerariesFU::SCRPN_VXUnit & Units)
    return ARM_MC::PipeType_Scorpion_VX;

  if (ScorpionItinerariesFU::SCRPN_Branch & Units)
    return ARM_MC::PipeType_Scorpion_X_Y;

  if (ScorpionItinerariesFU::SCRPN_MUX0 & Units)
    return ARM_MC::PipeType_Scorpion_VS; // DP/SP MOV to GPR only models MUX0

  // Scorpion itineraries should always fall into the above conditions.
  // This situation is not possible
  DEBUG(errs() << "Units=" << Units << "\n");
  llvm_unreachable("unexpected Scorpion instruction itinerary: "
                   "could not determine pipe type");
}

// Instruction pipeline affinity for Krait2 processor.
ARM_MC::PipeType ARM_MC::getKrait2PipeType(unsigned Units)
{
  // Krait Instruction Pipeline affinity
  // L pipe (Integer mem): uses LSUnit and L
  // S pipe (Integer mem): uses LSUnit and S
  // X_Y_M_B_Z pipe (Integer non-mem): uses Y, M, X, Z, or B
  // VL pipe (Neon/VFP mem and non-mem): uses LSUnit and VL, or uses VL
  // VS pipe (Neon/VFP mem and non-mem): uses LSUnit and VS, or uses VS
  // VX pipe (Neon/VFP Execute Pipe): uses VX

  // Return pipe unknown for empty itineraries.
  if (0 == Units)
    return ARM_MC::PipeType_Unknown;

  if (Krait2ItinerariesFU::KRT2_LSUnit & Units){
    // can be Integer or Neon/VFP
    if (Krait2ItinerariesFU::KRT2_L & Units)
      return ARM_MC::PipeType_Krait_L;
    if (Krait2ItinerariesFU::KRT2_S & Units)
      return ARM_MC::PipeType_Krait_S;
    if (Krait2ItinerariesFU::KRT2_VL & Units)
      return ARM_MC::PipeType_Krait_VL;
    if (Krait2ItinerariesFU::KRT2_VS & Units)
      return ARM_MC::PipeType_Krait_VS;
  }

  if (Krait2ItinerariesFU::KRT2_VS & Units)
    return ARM_MC::PipeType_Krait_VS; // VS non-mem

  if (Krait2ItinerariesFU::KRT2_VL & Units)
    return ARM_MC::PipeType_Krait_VL; // VL non-mem

  if (Krait2ItinerariesFU::KRT2_VX & Units)
    return ARM_MC::PipeType_Krait_VX;

  if ((Krait2ItinerariesFU::KRT2_X & Units) ||
      (Krait2ItinerariesFU::KRT2_Y & Units) ||
      (Krait2ItinerariesFU::KRT2_M & Units) ||
      (Krait2ItinerariesFU::KRT2_B & Units) ||
      (Krait2ItinerariesFU::KRT2_Z & Units))
    return ARM_MC::PipeType_Krait_X_Y_M_B_Z;

  if (Krait2ItinerariesFU::KRT2_MUX0 & Units)
    return ARM_MC::PipeType_Krait_VS; // DP/SP MOV to GPR only models MUX0

  // Krait itineraries should always fall into the above conditions.
  // This situation is not possible
  DEBUG(errs() << "Units=" << Units << "\n");
  llvm_unreachable("unexpected Krait2 instruction itinerary: "
                   "could not determine pipe type");
}


// Instruction pipeline affinity for Krait3 processor.
ARM_MC::PipeType ARM_MC::getKrait3PipeType(unsigned Units)
{
  // Krait Instruction Pipeline affinity
  // L pipe (Integer mem): uses LSUnit and L
  // S pipe (Integer mem): uses LSUnit and S
  // X_Y_M_B_Z pipe (Integer non-mem): uses Y, M, X, Z, or B
  // VL pipe (Neon/VFP mem and non-mem): uses LSUnit and VL, or uses VL
  // VS pipe (Neon/VFP mem and non-mem): uses LSUnit and VS, or uses VS
  // VX pipe (Neon/VFP Execute Pipe): uses VX

  // Return pipe unknown for empty itineraries.
  if (0 == Units)
    return ARM_MC::PipeType_Unknown;

  if (Krait3ItinerariesFU::KRT3_LSUnit & Units){
    // can be Integer or Neon/VFP
    if (Krait3ItinerariesFU::KRT3_L & Units)
      return ARM_MC::PipeType_Krait_L;
    if (Krait3ItinerariesFU::KRT3_S & Units)
      return ARM_MC::PipeType_Krait_S;
    if (Krait3ItinerariesFU::KRT3_VL & Units)
      return ARM_MC::PipeType_Krait_VL;
    if (Krait3ItinerariesFU::KRT3_VS & Units)
      return ARM_MC::PipeType_Krait_VS;
  }

  if (Krait3ItinerariesFU::KRT3_VS & Units)
    return ARM_MC::PipeType_Krait_VS; // VS non-mem

  if (Krait3ItinerariesFU::KRT3_VL & Units)
    return ARM_MC::PipeType_Krait_VL; // VL non-mem

  if (Krait3ItinerariesFU::KRT3_VX & Units)
    return ARM_MC::PipeType_Krait_VX;

  if ((Krait3ItinerariesFU::KRT3_Y & Units) ||
      (Krait3ItinerariesFU::KRT3_M & Units) ||
      (Krait3ItinerariesFU::KRT3_X & Units) ||
      (Krait3ItinerariesFU::KRT3_Z & Units) ||
      (Krait3ItinerariesFU::KRT3_B & Units))
    return ARM_MC::PipeType_Krait_X_Y_M_B_Z;

  if (Krait3ItinerariesFU::KRT3_MUX0 & Units)
    return ARM_MC::PipeType_Krait_VS; // DP/SP MOV to GPR only models MUX0

  // Krait itineraries should always fall into the above conditions.
  // This situation is not possible
  DEBUG(errs() << "Units=" << Units << "\n");
  llvm_unreachable("unexpected Krait3 instruction itinerary: "
                   "could not determine pipe type");
}


unsigned int ARM_MC::getKrait2PipeCount(unsigned Units)
{
  // Most instructions are executed in one specific pipe/FU.
  // But some integer instructions can be executed in more than one pipe/FU:
  // - KRT_X, KRT_Y, KRT_B and KRT_Z (4 pipes)
  // - KRT_X, KRT_Y and KRT_Z (3 pipes)
  // - KRT_X, KRT_Y (2 pipes)

  // Return high pipe count for empty itineraries.
  if (0 == Units)
    return 0xFFFF;

  if ((Krait2ItinerariesFU::KRT2_X & Units) &&
      (Krait2ItinerariesFU::KRT2_Y & Units) &&
      (Krait2ItinerariesFU::KRT2_B & Units) &&
      (Krait2ItinerariesFU::KRT2_Z & Units))
    return 4;

  if ((Krait2ItinerariesFU::KRT2_X & Units) &&
      (Krait2ItinerariesFU::KRT2_Y & Units) &&
      (Krait2ItinerariesFU::KRT2_Z & Units))
    return 3;

  if ((Krait2ItinerariesFU::KRT2_X & Units) &&
      (Krait2ItinerariesFU::KRT2_Y & Units) &&
      (Krait2ItinerariesFU::KRT2_B & Units))
    return 3;

  if ((Krait2ItinerariesFU::KRT2_X & Units) &&
      (Krait2ItinerariesFU::KRT2_Y & Units))
    return 2;

  return 1;
}

unsigned int ARM_MC::getKrait3PipeCount(unsigned Units)
{
  // Most instructions are executed in one specific pipe/FU.
  // But some integer instructions can be executed in more than one pipe/FU:
  // - KRT_X, KRT_Y, KRT_B and KRT_Z (4 pipes)
  // - KRT_X, KRT_Y and KRT_Z (3 pipes)
  // - KRT_X, KRT_Y (2 pipes)

  // Return high pipe count for empty itineraries.
  if (0 == Units)
    return 0xFFFF;

  if ((Krait3ItinerariesFU::KRT3_X & Units) &&
      (Krait3ItinerariesFU::KRT3_Y & Units) &&
      (Krait3ItinerariesFU::KRT3_B & Units) &&
      (Krait3ItinerariesFU::KRT3_Z & Units))
    return 4;

  if ((Krait3ItinerariesFU::KRT3_X & Units) &&
      (Krait3ItinerariesFU::KRT3_Y & Units) &&
      (Krait3ItinerariesFU::KRT3_Z & Units))
    return 3;

  if ((Krait3ItinerariesFU::KRT3_X & Units) &&
      (Krait3ItinerariesFU::KRT3_Y & Units) &&
      (Krait3ItinerariesFU::KRT3_B & Units))
    return 3;

  if ((Krait3ItinerariesFU::KRT3_X & Units) &&
      (Krait3ItinerariesFU::KRT3_Y & Units))
    return 2;

  return 1;
}


unsigned int ARM_MC::getScorpionPipeCount(unsigned Units)
{
  // Most instructions are executed in one specific pipe/FU.
  // But some integer instructions can be executed in more than one pipe/FU:
  // SCPRN_ALUX, SCPRN_ALUY - (2 pipes)

  // Return high pipe count for empty itineraries.
  if (0 == Units)
    return 0xFFFF;

  if ((ScorpionItinerariesFU::SCRPN_ALUX & Units) &&
      (ScorpionItinerariesFU::SCRPN_ALUY & Units))
    return 2;

  return 1;
}
