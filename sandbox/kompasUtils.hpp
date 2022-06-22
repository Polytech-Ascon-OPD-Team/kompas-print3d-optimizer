#pragma once

#import <kAPI5.tlb> no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

bool isKompasInstalled();
bool isKompasRun();
KompasObjectPtr kompasInit();
