#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace
{
constexpr uint64_t GUEST_BIAS = 1ULL << 40;
constexpr uint32_t GUEST_LOWEST = 0x40000000;
constexpr unsigned HOST_SPACE = 272;

struct Rebase
{
	Module &module;
	Type *i32, *i64;
	PointerType *host;
	InlineAsm *barrier;

	explicit Rebase(Module &m)
		: module(m), i32(Type::getInt32Ty(m.getContext())), i64(Type::getInt64Ty(m.getContext())),
		host(PointerType::get(m.getContext(), HOST_SPACE)),
		barrier(InlineAsm::get(FunctionType::get(i64, {i64}, false), "and $0, $1, #0xffffffff", "=r,r", false))
	{
	}

	Value *address(IRBuilder<> &builder, Value *pointer)
	{
		Value *value = builder.CreateZExt(builder.CreatePtrToInt(pointer, i32), i64);

		if (!isa<Constant>(value))
			value = builder.CreateCall(barrier, {value});
		return builder.CreateIntToPtr(builder.CreateOr(value, GUEST_BIAS), host);
	}

	Value *argument(IRBuilder<> &builder, Value *pointer)
	{
		Value *value = builder.CreatePtrToInt(pointer, i32);
		Value *outside = builder.CreateICmpULT(value, builder.getInt32(GUEST_LOWEST));

		return builder.CreateOr(builder.CreateCall(barrier, {builder.CreateZExt(value, i64)}),
			builder.CreateSelect(outside, builder.getInt64(0), builder.getInt64(GUEST_BIAS)));
	}

	void operand(Instruction *instruction, unsigned index)
	{
		IRBuilder<> builder(instruction);

		if (instruction->getOperand(index)->getType()->getPointerAddressSpace() == 0)
			instruction->setOperand(index, address(builder, instruction->getOperand(index)));
	}

	void library(MemIntrinsic *intrinsic)
	{
		IRBuilder<> builder(intrinsic);
		Type *pointer = intrinsic->getRawDest()->getType();
		Value *size = builder.CreateZExtOrTrunc(intrinsic->getLength(), i32);
		CallInst *call;

		if (auto *set = dyn_cast<MemSetInst>(intrinsic))
		{
			FunctionCallee memset = module.getOrInsertFunction("memset", pointer, pointer, i32, i32);

			call = builder.CreateCall(memset, {set->getRawDest(), builder.CreateZExt(set->getValue(), i32), size});
		}
		else
		{
			FunctionCallee copy = module.getOrInsertFunction(isa<MemMoveInst>(intrinsic) ? "memmove" : "memcpy",
				pointer, pointer, pointer, i32);

			call = builder.CreateCall(copy, {intrinsic->getRawDest(), cast<MemTransferInst>(intrinsic)->getRawSource(),
				size});
		}
		call->addFnAttr(Attribute::NoBuiltin);
		intrinsic->eraseFromParent();
	}

	void variable_argument(VAArgInst *instruction)
	{
		const DataLayout &layout = module.getDataLayout();
		Type *type = instruction->getType();
		uint64_t alignment = std::max<uint64_t>(4, layout.getABITypeAlign(type).value());
		IRBuilder<> builder(instruction);
		Value *list = address(builder, instruction->getPointerOperand());
		Value *slot = builder.CreatePtrToInt(builder.CreateLoad(builder.getPtrTy(), list), i32);

		slot = builder.CreateAnd(builder.CreateAdd(slot, builder.getInt32(alignment - 1)), ~(uint32_t)(alignment - 1));
		builder.CreateStore(builder.CreateIntToPtr(builder.CreateAdd(slot,
			builder.getInt32(alignTo(layout.getTypeAllocSize(type), 4))), builder.getPtrTy()), list);
		instruction->replaceAllUsesWith(builder.CreateLoad(type, address(builder, builder.CreateIntToPtr(slot,
			builder.getPtrTy()))));
		instruction->eraseFromParent();
	}

	void intrinsic(IntrinsicInst *intrinsic)
	{
		Intrinsic::ID id = intrinsic->getIntrinsicID();
		SmallVector<Value *, 2> arguments;
		IRBuilder<> builder(intrinsic);

		if (id != Intrinsic::vastart && id != Intrinsic::vacopy && id != Intrinsic::stackrestore)
			return;
		for (Value *argument : intrinsic->args())
			arguments.push_back(address(builder, argument));
		builder.CreateCall(Intrinsic::getOrInsertDeclaration(&module, id, {host}), arguments);
		intrinsic->eraseFromParent();
	}

	void call(CallBase *call)
	{
		Function *callee = call->getCalledFunction();
		IRBuilder<> builder(call);

		if (call->isInlineAsm())
			return;
		call->addFnAttr(Attribute::NoBuiltin);
		if (!callee)
		{
			call->setCalledOperand(address(builder, call->getCalledOperand()));
			return;
		}
		if (callee->isDeclaration() && callee->getName().starts_with("host"))
		{
			SmallVector<Type *, 8> types(call->getFunctionType()->params());

			for (unsigned index = 0; index < call->arg_size(); index++)
			{
				if (call->getArgOperand(index)->getType()->isPointerTy() &&
					call->getArgOperand(index)->getType()->getPointerAddressSpace() == 0)
				{
					call->setArgOperand(index, argument(builder, call->getArgOperand(index)));
					types[index] = i64;
				}
			}
			call->mutateFunctionType(FunctionType::get(call->getType(), types, call->getFunctionType()->isVarArg()));
		}
	}

	void run(Function &function)
	{
		SmallVector<Instruction *, 256> work;

		for (Instruction &instruction : instructions(function))
			work.push_back(&instruction);
		for (Instruction *instruction : work)
		{
			if (auto *load = dyn_cast<LoadInst>(instruction))
				operand(load, load->getPointerOperandIndex());
			else if (auto *store = dyn_cast<StoreInst>(instruction))
				operand(store, store->getPointerOperandIndex());
			else if (auto *rmw = dyn_cast<AtomicRMWInst>(instruction))
				operand(rmw, rmw->getPointerOperandIndex());
			else if (auto *exchange = dyn_cast<AtomicCmpXchgInst>(instruction))
				operand(exchange, exchange->getPointerOperandIndex());
			else if (auto *argument = dyn_cast<VAArgInst>(instruction))
				variable_argument(argument);
			else if (auto *memory = dyn_cast<MemIntrinsic>(instruction))
				library(memory);
			else if (auto *other = dyn_cast<IntrinsicInst>(instruction))
				intrinsic(other);
			else if (auto *other_call = dyn_cast<CallBase>(instruction))
				call(other_call);
		}
	}
};

struct GuestRebasePass : PassInfoMixin<GuestRebasePass>
{
	PreservedAnalyses run(Module &module, ModuleAnalysisManager &)
	{
		const DataLayout &layout = module.getDataLayout();
		Rebase rebase(module);

		if (layout.getPointerSize(0) != 4 || layout.getPointerSize(HOST_SPACE) != 8)
			report_fatal_error("halo-rebase: the guest needs 32-bit pointers and a 64-bit address space 272");
		for (Function &function : module)
		{
			if (!function.isDeclaration())
				rebase.run(function);
		}
		return PreservedAnalyses::none();
	}
};
}

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
{
	return {LLVM_PLUGIN_API_VERSION, "halo-rebase", "1", [](PassBuilder &builder)
	{
		builder.registerOptimizerLastEPCallback([](ModulePassManager &passes, OptimizationLevel, ThinOrFullLTOPhase)
		{
			passes.addPass(GuestRebasePass());
		});
	}};
}
