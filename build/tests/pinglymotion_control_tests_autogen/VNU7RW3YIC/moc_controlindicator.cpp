/****************************************************************************
** Meta object code from reading C++ file 'controlindicator.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/controlindicator.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'controlindicator.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_StatusPill_t {
    uint offsetsAndSizes[2];
    char stringdata0[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_StatusPill_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_StatusPill_t qt_meta_stringdata_StatusPill = {
    {
        QT_MOC_LITERAL(0, 10)   // "StatusPill"
    },
    "StatusPill"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_StatusPill[] = {

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

Q_CONSTINIT const QMetaObject StatusPill::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_StatusPill.offsetsAndSizes,
    qt_meta_data_StatusPill,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_StatusPill_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<StatusPill, std::true_type>
    >,
    nullptr
} };

void StatusPill::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *StatusPill::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StatusPill::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StatusPill.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int StatusPill::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_stringdata_ControlIndicator_t {
    uint offsetsAndSizes[20];
    char stringdata0[17];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[8];
    char stringdata5[10];
    char stringdata6[7];
    char stringdata7[8];
    char stringdata8[3];
    char stringdata9[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_ControlIndicator_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_ControlIndicator_t qt_meta_stringdata_ControlIndicator = {
    {
        QT_MOC_LITERAL(0, 16),  // "ControlIndicator"
        QT_MOC_LITERAL(17, 15),  // "toggleRequested"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 10),  // "setRunning"
        QT_MOC_LITERAL(45, 7),  // "running"
        QT_MOC_LITERAL(53, 9),  // "onContact"
        QT_MOC_LITERAL(63, 6),  // "client"
        QT_MOC_LITERAL(70, 7),  // "command"
        QT_MOC_LITERAL(78, 2),  // "ok"
        QT_MOC_LITERAL(81, 9)   // "onRefused"
    },
    "ControlIndicator",
    "toggleRequested",
    "",
    "setRunning",
    "running",
    "onContact",
    "client",
    "command",
    "ok",
    "onRefused"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_ControlIndicator[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    1,   39,    2, 0x0a,    2 /* Public */,
       5,    3,   42,    2, 0x0a,    4 /* Public */,
       9,    0,   49,    2, 0x0a,    8 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Bool,    6,    7,    8,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject ControlIndicator::staticMetaObject = { {
    QMetaObject::SuperData::link<StatusPill::staticMetaObject>(),
    qt_meta_stringdata_ControlIndicator.offsetsAndSizes,
    qt_meta_data_ControlIndicator,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_ControlIndicator_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ControlIndicator, std::true_type>,
        // method 'toggleRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setRunning'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onContact'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onRefused'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ControlIndicator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ControlIndicator *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->toggleRequested(); break;
        case 1: _t->setRunning((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->onContact((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 3: _t->onRefused(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ControlIndicator::*)();
            if (_t _q_method = &ControlIndicator::toggleRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *ControlIndicator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ControlIndicator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ControlIndicator.stringdata0))
        return static_cast<void*>(this);
    return StatusPill::qt_metacast(_clname);
}

int ControlIndicator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusPill::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ControlIndicator::toggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
namespace {
struct qt_meta_stringdata_AuroraIndicator_t {
    uint offsetsAndSizes[12];
    char stringdata0[16];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[10];
    char stringdata4[8];
    char stringdata5[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_AuroraIndicator_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_AuroraIndicator_t qt_meta_stringdata_AuroraIndicator = {
    {
        QT_MOC_LITERAL(0, 15),  // "AuroraIndicator"
        QT_MOC_LITERAL(16, 15),  // "toggleRequested"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 9),  // "setStatus"
        QT_MOC_LITERAL(43, 7),  // "enabled"
        QT_MOC_LITERAL(51, 9)   // "connected"
    },
    "AuroraIndicator",
    "toggleRequested",
    "",
    "setStatus",
    "enabled",
    "connected"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_AuroraIndicator[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   26,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    2,   27,    2, 0x0a,    2 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,    4,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject AuroraIndicator::staticMetaObject = { {
    QMetaObject::SuperData::link<StatusPill::staticMetaObject>(),
    qt_meta_stringdata_AuroraIndicator.offsetsAndSizes,
    qt_meta_data_AuroraIndicator,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_AuroraIndicator_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AuroraIndicator, std::true_type>,
        // method 'toggleRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void AuroraIndicator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AuroraIndicator *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->toggleRequested(); break;
        case 1: _t->setStatus((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AuroraIndicator::*)();
            if (_t _q_method = &AuroraIndicator::toggleRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *AuroraIndicator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AuroraIndicator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AuroraIndicator.stringdata0))
        return static_cast<void*>(this);
    return StatusPill::qt_metacast(_clname);
}

int AuroraIndicator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusPill::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void AuroraIndicator::toggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
namespace {
struct qt_meta_stringdata_TipsSwitch_t {
    uint offsetsAndSizes[10];
    char stringdata0[11];
    char stringdata1[8];
    char stringdata2[1];
    char stringdata3[3];
    char stringdata4[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_TipsSwitch_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_TipsSwitch_t qt_meta_stringdata_TipsSwitch = {
    {
        QT_MOC_LITERAL(0, 10),  // "TipsSwitch"
        QT_MOC_LITERAL(11, 7),  // "toggled"
        QT_MOC_LITERAL(19, 0),  // ""
        QT_MOC_LITERAL(20, 2),  // "on"
        QT_MOC_LITERAL(23, 5)   // "setOn"
    },
    "TipsSwitch",
    "toggled",
    "",
    "on",
    "setOn"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_TipsSwitch[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   26,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   29,    2, 0x0a,    3 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,

       0        // eod
};

Q_CONSTINIT const QMetaObject TipsSwitch::staticMetaObject = { {
    QMetaObject::SuperData::link<StatusPill::staticMetaObject>(),
    qt_meta_stringdata_TipsSwitch.offsetsAndSizes,
    qt_meta_data_TipsSwitch,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_TipsSwitch_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TipsSwitch, std::true_type>,
        // method 'toggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setOn'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void TipsSwitch::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TipsSwitch *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->setOn((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TipsSwitch::*)(bool );
            if (_t _q_method = &TipsSwitch::toggled; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *TipsSwitch::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TipsSwitch::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TipsSwitch.stringdata0))
        return static_cast<void*>(this);
    return StatusPill::qt_metacast(_clname);
}

int TipsSwitch::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusPill::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void TipsSwitch::toggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
