#include "FontVisitor.h"

#include <QSettings>
#include <QStringList>

#include <osgText/Font>
#include <osgText/Text>

FontVisitor::FontVisitor( osgText::Font* font, float size)
	: NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN ) 
{
	setFont(font, size);
}

FontVisitor::FontVisitor(QFontInfo& font)
	: NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN ) 
{
	setFont(font);
}

void FontVisitor::apply( osg::Geode &geode )
{
	for( unsigned int geodeIdx = 0; geodeIdx <  geode.getNumDrawables(); geodeIdx++ ) {
		if ( strcmp(geode.getDrawable(geodeIdx)->className(), "Text") == 0)
		{
			osgText::Text *curText = (osgText::Text*)geode.getDrawable( geodeIdx );
			curText->setCharacterSize(_size);
		}
	}
} 

void FontVisitor::setFont(QFontInfo& fontInfo)
{
	QString fontName = fontInfo.family();
	if (fontInfo.bold())	fontName.append("b");
	if (fontInfo.italic())	fontName.append("i");
	if (!(_font = osgText::readFontFile((fontName + ".ttf").toStdString())).valid())
		if (!(_font = osgText::readFontFile((fontName + ".ttc").toStdString())).valid())
			_font = osgText::Font::getDefaultFont();
	_font->setMinFilterHint(osg::Texture::LINEAR);
	_font->setMagFilterHint(osg::Texture::LINEAR);
	_size = fontInfo.pointSizeF();
}

void FontVisitor::setFont(osgText::Font* font, float size /*= 25.0f*/)
{
	if (font != NULL)
		_font = font;
	else
	{
		_font = osgText::Font::getDefaultFont();
		_font->setMinFilterHint(osg::Texture::LINEAR);
		_font->setMagFilterHint(osg::Texture::LINEAR);
	}
	_size = size;
}
