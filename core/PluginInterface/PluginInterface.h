#pragma once
#include "PluginInterface_global.h"
#include <MousePicker/MousePicker.h>
#include "../../NameSpace.h"

#include <QVariant>
#include <QMap>
#include <osgEarth/Viewpoint>

QT_BEGIN_NAMESPACE
class QToolBar;
class QMenu;
class QTreeWidgetItem;
class QAction;
class QActionGroup;
class QFontInfo;
QT_END_NAMESPACE

class ColorVisitor;
class FontVisitor;
namespace osgText
{
class Text;
class Font;
}
namespace osg
{
class LineWidth;
class Point;
}

namespace osgEarth 
{
	class GeoExtent;
	class Layer;
}

/** Common interface for plugins
 *
 * There are 3 main usages of a plugin:
 * 1. Ui manipulation
 *    - Every plugin is responsible to provide their Ui entrance (if any) in the main Ui.
 *    - Typically it is done in setupUi() which is called during plugin loading.
 *    - You can also access the main Ui through _mainWindow any time & any where.
 * 2. Interactive drawing
 *    - You can hook your interaction methods in event handlers, like onLeftButton().
 *    - Refer to _isDrawing to check drawing status, use beginDrawing() and endDrawing() to contrain a drawing action.
 *    - Always use _currentDrawNode to contain your drawn nodes, and add _currentDrawNode to _currentAnchor.
 *    - Use _anchoredWorldCoord as the current coord of mouse which is relative to _currentAnchor.
 *    - If any of your nodes is in true world coord (eg. not anchored), you should make it anchored by using a transform.
 * 3. Data management
 *    - For osg nodes, add them to _currentAnchor, or _pluginRoot if it is needed.
 *    - For osgEarth nodes, you should subclass EarthDataInterface and see instructions there.
 *    - You should call recordNode() for any data that you want the DataManager to manage for you.
 *    - Typically it is recommend to add data to _currentAnchor with anchored coord, so as to reduce jittering.
 *
 * Explanation of the anchor machanism:
 *   Large coordinates is subject to jittering in rendering, osg recommends using multilevel transform to reduce jittering.
 *   We design a grid of anchor points for the node to hook to, so as to contain a small local coord.
 *   Every time you call beginDrawing() or updateAnchorPoint(), _currentAnchor will be updated to the nearest anchor.
 *   Add your data to _currentAnchor whenever possible, and make sure their coord is relative to the _currentAnchor.
 *   You can use _anchoredOffset to transform any world coord to anchored coord
 */
class PLUGININTERFACE_EXPORT  PluginInterface: public MousePicker
{
	Q_OBJECT

public:
	struct StyleConfig
	{
		StyleConfig();

    osg::ref_ptr<osg::LineWidth>  lineWidth;
    osg::ref_ptr<osg::Point>      pointSize;
    osg::Vec4                     lineColor;
    osg::Vec4                     pointColor;
    osg::Vec4                     fillColor;
    osg::ref_ptr<osgText::Font>   font;
    float                         textSize;
    float                         textFloating;
    osg::Vec4                     textColor;
	};

public:
	PluginInterface();

	virtual ~PluginInterface(void);

  virtual void  init();

  virtual void  setupUi(QToolBar *toolBar, QMenu *menu) = 0;

	// Load a custom context menu when right click on the related tree node from DataTree
  virtual void  loadContextMenu(QMenu *contextMenu, QTreeWidgetItem *selectedItem);

	// Main function to handle input events
  virtual bool  handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa);

  QString       getPluginName();

  QString       getPluginGroup();

	/** Register an action as exclusive with others
   * If not set, the action can act with other mutex actions
   */
  static void   registerMutexAction(QAction *action);

	// Set an action that is always enabled when no other action is enabled
  static void   setDefaultAction(QAction *action);

protected:
  // Get or add a setting from the plugin settings
  // The setting is plugin-specific, so different plugin may have the same setting
  // If you don't provide a default value (or provide an invalid value), the setting won't be added
  QVariant                             getOrAddPluginSettings(const QString& key, const QVariant &defaultValue = QVariant());

  // Change a setting from the plugin settings
  // The setting is plugin-specific, so different plugin may have the same setting
  QVariant                             setPluginSettings(const QString& key, const QVariant &value);

  virtual osg::ref_ptr<osg::Geometry>  createPointGeode(const osg::Vec3 &pos, const osg::Vec3 &norm = osg::Vec3(0, 0, 1.0));

  virtual osg::ref_ptr<osgText::Text>  createTextGeode(std::string &txt, osg::Vec3 position);

	// Register an action with a mutex group where only one action can be enabled at one time
  static void                          uncheckMutexActions(QAction *action);

	// Style config
  StyleConfig                          getDefaultStyle();

	// Record current draw node to DataTree
  virtual void                         recordCurrent(const QString& parent = "");

	// Record a node to DataTree
  virtual void                         recordNode(osg::Node *node, const QString& name = "", const QString& parent = "");

	// Record a layer to DataTree
	virtual void recordLayer(osgEarth::Layer* layer, const QString& name = "", const QString& parent = "");

  /** Call this before you begin a new drawing
   * It does the following things:
   *   1. update _currentAnchor and _anchoredOffset
   *   2. update _isDrawing status
   */
  virtual void                         beginDrawing();

  /** Call this after you fishied a drawing
   * It makes sure any thing related to drawing do not happen afterwards
   */
  virtual void                         endDrawing();

	/** Update _currentAnchor to the nearest anchor point
   *
   * This function exists to support anti-jittering for large coordinates
   */
  void                                 updateAnchorPoint();

	/** Return the nearest anchor point to the target (default as _currentWorldPos)
   * The target should be in world coordinate
   * This function exists to support anti-jittering for large coordinates
   */
  osg::PositionAttitudeTransform     * getNearestAnchorPoint(const osg::Vec3 &point = _currentWorldPos);

  // Return an anchored version of a world space array
  osg::ref_ptr<osg::Vec3Array>         anchorArray(osg::ref_ptr<osg::Vec3Array> worldSpaceArray);

	// -------------------------Keyboard and mouse callback functions-------------------------
  virtual void                         onLeftButton()
  {
  }

  virtual void                         onRightButton()
  {
  }

  virtual void                         onReleaseButton()
  {
  }

  virtual void                         onDoubleClick()
  {
  }

  virtual void                         onMouseMove()
  {
  }

  virtual void                         onArrowKeyUp()
  {
  }

  virtual void                         onArrowKeyDown()
  {
  }

signals:
	void recordData(osg::Node*, QString, QString, bool = false);
	void recordData(osgEarth::Layer*, QString, QString, osgEarth::GeoExtent* = NULL, bool = false);
	void removeData(const QString&);
	void switchData(QString, bool);
	void loadingProgress(int);
	void loadingDone();
  void setViewPoint(const osgEarth::Viewpoint &);

public slots:
	// Default function to toggle the plugin on or off
  virtual void  toggle(bool checked = true);

protected:
	// Indicator of drawing status
  bool  _isDrawing;

	// Number of nodes that have been drawn
  int  _instanceCount;

	// A category name for all drawings done by this PluginInterface
  QString  _pluginName;

	// A category name to determine where the plugin's UI should be loaded
	// You can only choose from { "Data", "Draw", "Measure", "Analysis", "Effect", "Edit"}
  QString  _pluginCategory;

	// Root node for all drawings of this PluginInterface
  osg::ref_ptr<osg::Group>  _pluginRoot;

	/** Root node for all drawings of this time.
   * Every time a draw action begins, a new _currentDrawNode should be instantiated
   */
  osg::ref_ptr<osg::Geode>  _currentDrawNode;

	// Anchor points that help to reduce maltitude of coordinates
	// This helps to reduce node jittering
  QMap<unsigned, QMap<unsigned, osg::ref_ptr<osg::PositionAttitudeTransform>>>  anchorPoints;

	/** Root node with small local coords
   * Every time you add a node to the scene, add it to the current anchor
   * OSGEarth layers are not required to do so
   */
  osg::ref_ptr<osg::PositionAttitudeTransform>  _currentAnchor;

  osg::Vec3  _anchoredOffset;
  osg::Vec3  _anchoredWorldPos;

  osgViewer::View *_currentView;

  const osgGA::GUIEventAdapter *_currentEA;
  StyleConfig                   _style;

	// Draw root of the currently activated PluginInterface
  static osg::Group *_activatedPlugin;

private:
	// An action group that allow only one action to be toggled at any time
  static QActionGroup *_mutexActions;

	// A default action that is toggled when no other action is toggled
  static QAction *_defaultAction;
  double          _anchorStepX;
  double          _anchorStepY;
};

QT_BEGIN_NAMESPACE
#define PluginInterface_iid "io.tqjxlm.Atlas.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
QT_END_NAMESPACE
