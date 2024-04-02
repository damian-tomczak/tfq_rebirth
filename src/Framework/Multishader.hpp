/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef MULTISHADER_H_
#define MULTISHADER_H_

#include "Res_d3d.hpp"

namespace res
{

class Multishader_pimpl;

class Multishader : public D3dResource
{
public:
	struct SHADER_INFO
	{
		ID3DXEffect *Effect;
		std::vector<D3DXHANDLE> Params;

		// Pobiera i zachowuje sobie uchwyty parametr�w
		SHADER_INFO(ID3DXEffect *Effect, uint4 ParamCount, const STRING_VECTOR &ParamNames);
	};

	static void RegisterResourceType();
	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

	Multishader(
		const string &Name, const string &Group,
		const string &SourceFileName, const string &CacheFileNameMask,
		uint MacroCount, const char *MacroNames[], const uint MacroBits[],
		uint ParamCount, const char *ParamNames[]);
	Multishader(
		const string &Name, const string &Group,
		const string &SourceFileName, const string &CacheFileNameMask,
		const STRING_VECTOR &MacroNames, const uint MacroBits[],
		const STRING_VECTOR &ParamNames);
	~Multishader();

	// Zwraca liczb� aktualnie wczytanych shader�w
	uint GetShaderCount();
	// Przetwarza ustawienia makr shadera na �a�cuch
	void MacrosToStr(string *Out, uint Macros[]);

	// Zwraca, a w razie potrzeby wczytuje shader o podanych ustawieniach makr.
	// Je�li nie mo�na wczyta�, rzuca wyj�tek.
	// Mo�na wywo�ywa� tylko kiedy zas�b jest wczytany i urz�dzenie D3D nie jest utracone.
	const SHADER_INFO & GetShader(uint Macros[]);

protected:
	// ======== Implementacja IResource ========
	virtual void OnLoad();
	virtual void OnUnload();

	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

private:
	scoped_ptr<Multishader_pimpl> pimpl;
};

} // namespace res

#endif
