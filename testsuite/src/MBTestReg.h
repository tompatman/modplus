
#pragma once

#include "MBReg.h"
#include "util/IEnumPrinter.h"

typedef enum {
    TestRegs_Invalid = -1
    , TestRegs_RegUint16
    , TestRegs_RegUint32
    , TestRegs_RegStr
    , TestRegs_Num
} eTestRegs;


class TestRegPrint: public util::IEnumPrinter<eTestRegs> {
public:
    std::string getString(eTestRegs eVar) override {
      return("Unimplemented");
    };
};

//Test register that can be used to verify a call to the set handler
template <typename regIDs, typename valType>
class MBTestReg : modplus::MBReg<regIDs, valType> {
public:
	MBTestReg() {
		_bSetCalled = false;
	}

	virtual ~MBTestReg() {
	};

	virtual void setValue(){
		//inidcate this has been called
		_bSetCalled = true;
		//MBReg::setValue();
	};

	//Reset the flag after returning this once
	bool hasSetBeenCalled() {
		bool bSetCalled = _bSetCalled;
		_bSetCalled = false;
		return(bSetCalled);
	};

private:
	bool _bSetCalled;
};

