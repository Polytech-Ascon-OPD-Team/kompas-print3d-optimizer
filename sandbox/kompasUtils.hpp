#ifndef KOMPAS_UTILS_HPP
#define KOMPAS_UTILS_HPP

#import "ksconstants.tlb" no_namespace named_guids
#import "ksConstants3D.tlb" no_namespace named_guids
#import "kAPI5.tlb" no_namespace named_guids rename( "min", "Imin" ) rename( "max", "Imax" ) rename( "ksFragmentLibrary", "ksIFragmentLibrary" )
#import "kAPI7.tlb" no_namespace named_guids rename( "CreateWindow", "ICreateWindow" ) rename( "PostMessage", "IPostMessage" ) rename( "MessageBoxEx", "IMessageBoxEx" )

struct Sketch {
    ksEntityPtr entity;
    ksSketchDefinitionPtr definition;
    ksDocument2DPtr document2d;
    IKompasDocument2DPtr document2d_api7;
};

bool isKompasInstalled();
bool isKompasRun();
KompasObjectPtr kompasInit();

Sketch createSketch(KompasObjectPtr kompas, ksPartPtr part, ksFaceDefinitionPtr face);

#endif /* KOMPAS_UTILS_HPP */
