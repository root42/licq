#ifndef USERBOX_H
#define USERBOX_H

#include <vector>

#include <qlistview.h>
#include <qtooltip.h>
#if QT_VERSION < 300
  #include <qvector.h>
#else
  #include <qptrvector.h>
#endif

class ICQUser;
class CUserView;

enum BarType
{
  BAR_ONLINE,
  BAR_OFFLINE
};

enum FlashType
{
  FLASH_NONE,
  FLASH_ALL,
  FLASH_URGENT
};

class CColumnInfo
{
public:
  CColumnInfo(QString _sTitle, const char *_szFormat,
              unsigned short _nWidth, unsigned short _nAlign)
    { m_sTitle = _sTitle;
      m_szFormat = _szFormat;
      m_nWidth = _nWidth;
      m_nAlign = _nAlign; }
  ~CColumnInfo() {};

  QString m_sTitle;
  QCString m_szFormat;
  unsigned short m_nWidth, m_nAlign;
};

typedef std::vector<CColumnInfo *> ColumnInfos;


//=====UserViewItem================================================================================
class CUserViewItem : public QListViewItem
{
public:
  CUserViewItem (ICQUser *, QListView *);
  CUserViewItem (ICQUser *, CUserViewItem* item);
  CUserViewItem (BarType, QListView *);
  CUserViewItem (unsigned short Id, const char* name, QListView *);

  virtual ~CUserViewItem();
  virtual QString key(int column, bool ascending) const;
  unsigned long ItemUin() const { return m_nUin; }
#ifdef QT_PROTOCOL_PLUGIN
  char *ItemId() const { return m_szId; }
  unsigned long ItemPPID() const { return m_nPPID; }
#endif
  unsigned short GroupId() const { return m_nGroupId; }
#ifdef QT_PROTOCOL_PLUGIN
  bool isGroupItem() const { return (m_szId && m_nPPID && m_nGroupId != (unsigned short)(-1)); }
#else
  bool isGroupItem() const { return (m_nUin == 0 && m_nGroupId != (unsigned short)(-1)); }
#endif
  QCString  GroupName() const { return m_sGroupName; }
  void setGraphics(ICQUser *);
  unsigned short Status() const { return m_nStatus; };
  CUserViewItem* firstChild() const { return static_cast<CUserViewItem*>(QListViewItem::firstChild()); }
  CUserViewItem* nextSibling() const { return static_cast<CUserViewItem*>(QListViewItem::nextSibling()); }
  CUserView* listView() const { return (CUserView*) QListViewItem::listView(); }

protected:
  virtual void paintCell ( QPainter *, const QColorGroup &, int column, int width, int align);
  virtual void paintFocus ( QPainter *, const QColorGroup & cg, const QRect & r ) { };

  void drawCAROverlay(QPainter*);
  void SetThreadViewGroupTitle();

  QColor *m_cFore, *m_cBack;
  QPixmap *m_pIcon, *m_pIconStatus;

  unsigned long m_nUin;
#ifdef QT_PROTOCOL_PLUGIN
  char *m_szId;
  unsigned long m_nPPID;
#endif
  unsigned short m_nStatus;
  unsigned long m_nStatusFull;
  unsigned short m_nGroupId;
  int m_nOnlCount, m_nEvents;
  QCString m_sGroupName;
  bool m_bStatusInvisible, m_bCustomAR, m_bSecure, m_bFlash, m_bBirthday, m_bPhone, m_bCellular;
  QFont::Weight m_nWeight;
  bool m_bItalic, m_bStrike, m_bUrgent;
  QString m_sPrefix, m_sSortKey;

  static QColor  *s_cOnline,
                 *s_cAway,
                 *s_cOffline,
                 *s_cNew,
                 *s_cBack,
                 *s_cGridLines;

  friend class CUserView;
};

class CUserView;
#if QT_VERSION < 300
  typedef QVector<CUserView> UserFloatyList;
#else
  typedef QPtrVector<CUserView> UserFloatyList;
#endif


//=====UserView===============================================================
class CUserView : public QListView, public QToolTip
{
  Q_OBJECT
public:
  CUserView (QPopupMenu *m, QWidget *parent = 0, const char *name = 0);
  virtual ~CUserView();

  virtual void clear();

  CUserViewItem *firstChild() const { return (CUserViewItem *)QListView::firstChild(); }

  void setColors(char *_sOnline, char *_sAway, char *_sOffline,
                 char *_sNew, char *_sBack, char *_sGridLines);
  void setShowHeader(bool);
  void AnimationAutoResponseCheck(unsigned long uin);
  void AnimationOnline(unsigned long uin);
  unsigned long MainWindowSelectedItemUin();
#ifdef QT_PROTOCOL_PLUGIN
  void AnimationAutoResponseCheck(const char *, unsigned long);
  void AnimationOnline(const char *, unsigned long);
  bool MainWindowSelectedItemUser(char *&, unsigned long &);
#endif

  static UserFloatyList* floaties;
  static CUserView *FindFloaty(unsigned long);
#ifdef QT_PROTOCOL_PLUGIN
  static CUserView *FindFloaty(const char *, unsigned long);
#endif
  static void UpdateFloaties();

protected:
  int m_nFlashCounter;
  int msgTimerId;
  int onlTimerId, onlCounter;
  int carTimerId, carCounter;
  unsigned long carUin;
  unsigned long onlUin;
#ifdef QT_PROTOCOL_PLUGIN
  char *carId;
  char *onlId;
  unsigned long carPPID;
  unsigned long onlPPID;
#endif

  QPopupMenu *mnuUser;
  CUserViewItem *barOnline, *barOffline;
  QPoint mousePressPos;
  int numOnline, numOffline;
  virtual void timerEvent(QTimerEvent*);

  virtual void viewportMousePressEvent(QMouseEvent *e);
  virtual void viewportMouseReleaseEvent(QMouseEvent *e);
  virtual void viewportMouseMoveEvent(QMouseEvent * me);
  virtual void keyPressEvent(QKeyEvent *e);
  virtual void paintEmptyArea( QPainter *, const QRect & );

  virtual void viewportDragEnterEvent(QDragEnterEvent*);
  virtual void viewportDropEvent(QDropEvent*);
#if QT_VERSION >= 300
  virtual void contentsContextMenuEvent ( QContextMenuEvent * );
#endif

  virtual void resizeEvent(QResizeEvent *);
  virtual void maybeTip(const QPoint&);

  friend class CUserViewItem;
protected slots:
  void itemExpanded(QListViewItem*);
  void itemCollapsed(QListViewItem*);
};

#endif
