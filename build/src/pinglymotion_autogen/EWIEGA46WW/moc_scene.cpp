/****************************************************************************
** Meta object code from reading C++ file 'scene.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/scene.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scene.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_Scene_t {
    uint offsetsAndSizes[22];
    char stringdata0[6];
    char stringdata1[8];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[11];
    char stringdata5[10];
    char stringdata6[13];
    char stringdata7[3];
    char stringdata8[20];
    char stringdata9[4];
    char stringdata10[22];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Scene_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Scene_t qt_meta_stringdata_Scene = {
    {
        QT_MOC_LITERAL(0, 5),  // "Scene"
        QT_MOC_LITERAL(6, 7),  // "repaint"
        QT_MOC_LITERAL(14, 0),  // ""
        QT_MOC_LITERAL(15, 17),  // "animationSelected"
        QT_MOC_LITERAL(33, 10),  // "Animation*"
        QT_MOC_LITERAL(44, 9),  // "animation"
        QT_MOC_LITERAL(54, 12),  // "protectFrame"
        QT_MOC_LITERAL(67, 2),  // "on"
        QT_MOC_LITERAL(70, 19),  // "storeCameraPosition"
        QT_MOC_LITERAL(90, 3),  // "num"
        QT_MOC_LITERAL(94, 21)   // "restoreCameraPosition"
    },
    "Scene",
    "repaint",
    "",
    "animationSelected",
    "Animation*",
    "animation",
    "protectFrame",
    "on",
    "storeCameraPosition",
    "num",
    "restoreCameraPosition"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Scene[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,
       3,    1,   45,    2, 0x06,    2 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    1,   48,    2, 0x0a,    4 /* Public */,
       8,    1,   51,    2, 0x0a,    6 /* Public */,
      10,    1,   54,    2, 0x0a,    8 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Int,    9,

       0        // eod
};

Q_CONSTINIT const QMetaObject Scene::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Scene.offsetsAndSizes,
    qt_meta_data_Scene,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Scene_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Scene, std::true_type>,
        // method 'repaint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'animationSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<Animation *, std::false_type>,
        // method 'protectFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'storeCameraPosition'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'restoreCameraPosition'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void Scene::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Scene *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->repaint(); break;
        case 1: _t->animationSelected((*reinterpret_cast< std::add_pointer_t<Animation*>>(_a[1]))); break;
        case 2: _t->protectFrame((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->storeCameraPosition((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->restoreCameraPosition((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Scene::*)();
            if (_t _q_method = &Scene::repaint; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Scene::*)(Animation * );
            if (_t _q_method = &Scene::animationSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *Scene::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Scene::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Scene.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Scene::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void Scene::repaint()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Scene::animationSelected(Animation * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
