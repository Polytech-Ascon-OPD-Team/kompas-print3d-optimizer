#ifndef KOMPAS_UTILS_HPP
#define KOMPAS_UTILS_HPP


#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )

bool isKompasInstalled();
bool isKompasRun();
KompasObjectPtr kompasInit();


#endif /* KOMPAS_UTILS_HPP */
