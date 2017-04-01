//
//  CVar.cpp
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "CVar.h"
#include "main.h"

////////////////////////////////////////////////////////////////////////////////////////////
// LISTENER

void ICVarListener::AddRef(CVar* cv){ _refs.insert(cv); };

ICVarListener::~ICVarListener()
{
    STD_CONST_FOREACH(RefSet, _refs, r)
    {
        (*r)->RemoveListener(this);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////
// CVAR

CVar* CVar::s_first = NULL;
CVar::ListenerSet CVar::s_listeners;

void CVar::Init()
{
    // append to the link list
    _next = s_first;
    s_first = this;
}
