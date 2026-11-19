/****************************************************************************
** Meta object code from reading C++ file 'timeline.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/timeline.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'timeline.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_Timeline_t {
    uint offsetsAndSizes[38];
    char stringdata0[9];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[4];
    char stringdata4[13];
    char stringdata5[6];
    char stringdata6[8];
    char stringdata7[8];
    char stringdata8[17];
    char stringdata9[11];
    char stringdata10[5];
    char stringdata11[12];
    char stringdata12[6];
    char stringdata13[12];
    char stringdata14[16];
    char stringdata15[12];
    char stringdata16[23];
    char stringdata17[18];
    char stringdata18[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Timeline_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Timeline_t qt_meta_stringdata_Timeline = {
    {
        QT_MOC_LITERAL(0, 8),  // "Timeline"
        QT_MOC_LITERAL(9, 14),  // "positionCenter"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 3),  // "pos"
        QT_MOC_LITERAL(29, 12),  // "trackClicked"
        QT_MOC_LITERAL(42, 5),  // "track"
        QT_MOC_LITERAL(48, 7),  // "resized"
        QT_MOC_LITERAL(56, 7),  // "newSize"
        QT_MOC_LITERAL(64, 16),  // "animationChanged"
        QT_MOC_LITERAL(81, 10),  // "Animation*"
        QT_MOC_LITERAL(92, 4),  // "anim"
        QT_MOC_LITERAL(97, 11),  // "deleteFrame"
        QT_MOC_LITERAL(109, 5),  // "frame"
        QT_MOC_LITERAL(115, 11),  // "insertFrame"
        QT_MOC_LITERAL(127, 15),  // "setCurrentFrame"
        QT_MOC_LITERAL(143, 11),  // "redrawTrack"
        QT_MOC_LITERAL(155, 22),  // "redrawTrackImmediately"
        QT_MOC_LITERAL(178, 17),  // "setNumberOfFrames"
        QT_MOC_LITERAL(196, 6)   // "frames"
    },
    "Timeline",
    "positionCenter",
    "",
    "pos",
    "trackClicked",
    "track",
    "resized",
    "newSize",
    "animationChanged",
    "Animation*",
    "anim",
    "deleteFrame",
    "frame",
    "insertFrame",
    "setCurrentFrame",
    "redrawTrack",
    "redrawTrackImmediately",
    "setNumberOfFrames",
    "frames"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Timeline[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   74,    2, 0x06,    1 /* Public */,
       4,    1,   77,    2, 0x06,    3 /* Public */,
       6,    1,   80,    2, 0x06,    5 /* Public */,
       8,    1,   83,    2, 0x06,    7 /* Public */,
      11,    2,   86,    2, 0x06,    9 /* Public */,
      13,    2,   91,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      14,    1,   96,    2, 0x0a,   15 /* Public */,
      15,    1,   99,    2, 0x09,   17 /* Protected */,
      16,    1,  102,    2, 0x09,   19 /* Protected */,
      17,    1,  105,    2, 0x09,   21 /* Protected */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::QSize,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    5,   12,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    5,   12,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int,   18,

       0        // eod
};

Q_CONSTINIT const QMetaObject Timeline::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_Timeline.offsetsAndSizes,
    qt_meta_data_Timeline,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Timeline_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Timeline, std::true_type>,
        // method 'positionCenter'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'trackClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'resized'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QSize &, std::false_type>,
        // method 'animationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<Animation *, std::false_type>,
        // method 'deleteFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'insertFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setCurrentFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'redrawTrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'redrawTrackImmediately'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setNumberOfFrames'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void Timeline::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Timeline *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->positionCenter((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->trackClicked((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->resized((*reinterpret_cast< std::add_pointer_t<QSize>>(_a[1]))); break;
        case 3: _t->animationChanged((*reinterpret_cast< std::add_pointer_t<Animation*>>(_a[1]))); break;
        case 4: _t->deleteFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->insertFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->setCurrentFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->redrawTrack((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->redrawTrackImmediately((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->setNumberOfFrames((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Timeline::*)(int );
            if (_t _q_method = &Timeline::positionCenter; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Timeline::*)(int );
            if (_t _q_method = &Timeline::trackClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Timeline::*)(const QSize & );
            if (_t _q_method = &Timeline::resized; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Timeline::*)(Animation * );
            if (_t _q_method = &Timeline::animationChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Timeline::*)(int , int );
            if (_t _q_method = &Timeline::deleteFrame; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Timeline::*)(int , int );
            if (_t _q_method = &Timeline::insertFrame; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *Timeline::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Timeline::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Timeline.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int Timeline::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void Timeline::positionCenter(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Timeline::trackClicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Timeline::resized(const QSize & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Timeline::animationChanged(Animation * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Timeline::deleteFrame(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Timeline::insertFrame(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
