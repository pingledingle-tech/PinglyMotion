/****************************************************************************
** Meta object code from reading C++ file 'timelineview.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/timelineview.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'timelineview.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_TimelineTracks_t {
    uint offsetsAndSizes[2];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_TimelineTracks_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_TimelineTracks_t qt_meta_stringdata_TimelineTracks = {
    {
        QT_MOC_LITERAL(0, 14)   // "TimelineTracks"
    },
    "TimelineTracks"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_TimelineTracks[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject TimelineTracks::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TimelineTracks.offsetsAndSizes,
    qt_meta_data_TimelineTracks,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_TimelineTracks_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TimelineTracks, std::true_type>
    >,
    nullptr
} };

void TimelineTracks::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *TimelineTracks::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TimelineTracks::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TimelineTracks.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TimelineTracks::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_stringdata_TimelineView_t {
    uint offsetsAndSizes[22];
    char stringdata0[13];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[6];
    char stringdata4[18];
    char stringdata5[14];
    char stringdata6[9];
    char stringdata7[2];
    char stringdata8[13];
    char stringdata9[11];
    char stringdata10[5];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_TimelineView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_TimelineView_t qt_meta_stringdata_TimelineView = {
    {
        QT_MOC_LITERAL(0, 12),  // "TimelineView"
        QT_MOC_LITERAL(13, 11),  // "selectTrack"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 5),  // "track"
        QT_MOC_LITERAL(32, 17),  // "backgroundClicked"
        QT_MOC_LITERAL(50, 13),  // "layoutChanged"
        QT_MOC_LITERAL(64, 8),  // "scrollTo"
        QT_MOC_LITERAL(73, 1),  // "x"
        QT_MOC_LITERAL(75, 12),  // "setAnimation"
        QT_MOC_LITERAL(88, 10),  // "Animation*"
        QT_MOC_LITERAL(99, 4)   // "anim"
    },
    "TimelineView",
    "selectTrack",
    "",
    "track",
    "backgroundClicked",
    "layoutChanged",
    "scrollTo",
    "x",
    "setAnimation",
    "Animation*",
    "anim"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_TimelineView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   44,    2, 0x0a,    1 /* Public */,
       4,    0,   47,    2, 0x0a,    3 /* Public */,
       5,    0,   48,    2, 0x0a,    4 /* Public */,
       6,    1,   49,    2, 0x09,    5 /* Protected */,
       8,    1,   52,    2, 0x09,    7 /* Protected */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, 0x80000000 | 9,   10,

       0        // eod
};

Q_CONSTINIT const QMetaObject TimelineView::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_TimelineView.offsetsAndSizes,
    qt_meta_data_TimelineView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_TimelineView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TimelineView, std::true_type>,
        // method 'selectTrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'backgroundClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'layoutChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scrollTo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setAnimation'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<Animation *, std::false_type>
    >,
    nullptr
} };

void TimelineView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TimelineView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selectTrack((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->backgroundClicked(); break;
        case 2: _t->layoutChanged(); break;
        case 3: _t->scrollTo((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->setAnimation((*reinterpret_cast< std::add_pointer_t<Animation*>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *TimelineView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TimelineView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TimelineView.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int TimelineView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
