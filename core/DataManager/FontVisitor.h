#pragma once

#include <osg/NodeVisitor>

#include <QFontInfo>

namespace osgText {
	class Font;
}


class  FontVisitor : public osg::NodeVisitor 
{
public :
	FontVisitor( osgText::Font* font = NULL, float size = 25.0f);
	FontVisitor( QFontInfo& font);

	virtual ~FontVisitor(){}

	virtual void apply( osg::Geode &geode );

	void setFont(QFontInfo& fontInfo);

	void setFont(osgText::Font* font, float size = 25.0f);

private :
	osg::ref_ptr<osgText::Font> _font;
	float _size;
};