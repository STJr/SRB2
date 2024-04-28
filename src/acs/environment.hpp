// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Russell's Smart Interfaces
// Copyright (C) 2024 by Sonic Team Junior.
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  environment.hpp
/// \brief Action Code Script: Environment definition

#ifndef __SRB2_ACS_ENVIRONMENT_HPP__
#define __SRB2_ACS_ENVIRONMENT_HPP__

#include "acsvm.hpp"

namespace srb2::acs {

class Environment : public ACSVM::Environment
{
public:
	Environment();

	virtual bool checkTag(ACSVM::Word type, ACSVM::Word tag);

	virtual ACSVM::Word callSpecImpl(
		ACSVM::Thread *thread, ACSVM::Word spec,
		const ACSVM::Word *argV, ACSVM::Word argC
	);

	virtual ACSVM::Thread *allocThread();

	virtual void printKill(ACSVM::Thread *thread, ACSVM::Word type, ACSVM::Word data);

protected:
	virtual void loadModule(ACSVM::Module *module);

	virtual ACSVM::ModuleName getModuleName(char const *str, std::size_t len);
};

}

extern srb2::acs::Environment ACSEnv;

#endif // __SRB2_ACS_ENVIRONMENT_HPP__
