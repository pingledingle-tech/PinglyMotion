/****************************************************************************
** Meta object code from reading C++ file 'animation.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/animation.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'animation.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_Animation_t {
    uint offsetsAndSizes[30];
    char stringdata0[10];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[4];
    char stringdata4[13];
    char stringdata5[6];
    char stringdata6[13];
    char stringdata7[12];
    char stringdata8[6];
    char stringdata9[15];
    char stringdata10[6];
    char stringdata11[15];
    char stringdata12[12];
    char stringdata13[12];
    char stringdata14[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Animation_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Animation_t qt_meta_stringdata_Animation = {
    {
        QT_MOC_LITERAL(0, 9),  // "Animation"
        QT_MOC_LITERAL(10, 14),  // "numberOfFrames"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 3),  // "num"
        QT_MOC_LITERAL(30, 12),  // "currentFrame"
        QT_MOC_LITERAL(43, 5),  // "frame"
        QT_MOC_LITERAL(49, 12),  // "frameChanged"
        QT_MOC_LITERAL(62, 11),  // "redrawTrack"
        QT_MOC_LITERAL(74, 5),  // "track"
        QT_MOC_LITERAL(80, 14),  // "animationDirty"
        QT_MOC_LITERAL(95, 5),  // "state"
        QT_MOC_LITERAL(101, 14),  // "deleteKeyFrame"
        QT_MOC_LITERAL(116, 11),  // "jointNumber"
        QT_MOC_LITERAL(128, 11),  // "insertFrame"
        QT_MOC_LITERAL(140, 11)   // "deleteFrame"
    },
    "Animation",
    "numberOfFrames",
    "",
    "num",
    "currentFrame",
    "frame",
    "frameChanged",
    "redrawTrack",
    "track",
    "animationDirty",
    "state",
    "deleteKeyFrame",
    "jointNumber",
    "insertFrame",
    "deleteFrame"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Animation[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       4,    1,   65,    2, 0x06,    3 /* Public */,
       6,    0,   68,    2, 0x06,    5 /* Public */,
       7,    1,   69,    2, 0x06,    6 /* Public */,
       9,    1,   72,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    2,   75,    2, 0x0a,   10 /* Public */,
      13,    2,   80,    2, 0x0a,   13 /* Public */,
      14,    2,   85,    2, 0x0a,   16 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Bool,   10,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   12,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   12,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   12,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject Animation::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Animation.offsetsAndSizes,
    qt_meta_data_Animation,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Animation_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Animation, std::true_type>,
        // method 'numberOfFrames'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'currentFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'frameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'redrawTrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'animationDirty'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'deleteKeyFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'insertFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'deleteFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void Animation::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Animation *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->numberOfFrames((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->currentFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->frameChanged(); break;
        case 3: _t->redrawTrack((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->animationDirty((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->deleteKeyFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->insertFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->deleteFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Animation::*)(int );
            if (_t _q_method = &Animation::numberOfFrames; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Animation::*)(int );
            if (_t _q_method = &Animation::currentFrame; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Animation::*)();
            if (_t _q_method = &Animation::frameChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Animation::*)(int );
            if (_t _q_method = &Animation::redrawTrack; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Animation::*)(bool );
            if (_t _q_method = &Animation::animationDirty; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *Animation::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Animation::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Animation.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Animation::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void Animation::numberOfFrames(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Animation::currentFrame(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Animation::frameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Animation::redrawTrack(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Animation::animationDirty(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
