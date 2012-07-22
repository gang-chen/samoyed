// File type registry.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_FILE_TYPE_REGISTRY_HPP
#define SMYD_FILE_TYPE_REGISTRY_HPP

namespace Samoyed
{

class ParserFactory;

class FileTypeRegistry
{
public:
	void registerFileType();

	ParserFactory *getParserFactory(const char *fileName) const;
};

}

#endif
