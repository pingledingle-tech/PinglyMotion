/****************************************************************************
** Meta object code from reading C++ file 'aboutdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/aboutdialog.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'aboutdialog.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_AboutDialog_t {
    uint offsetsAndSizes[14];
    char stringdata0[12];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[20];
    char stringdata4[30];
    char stringdata5[36];
    char stringdata6[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_AboutDialog_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_AboutDialog_t qt_meta_stringdata_AboutDialog = {
    {
        QT_MOC_LITERAL(0, 11),  // "AboutDialog"
        QT_MOC_LITERAL(12, 12),  // "checkUpdates"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 19),  // "updateCheckFinished"
        QT_MOC_LITERAL(46, 29),  // "on_checkUpdatesButton_clicked"
        QT_MOC_LITERAL(76, 35),  // "on_autoUpdatesCheckBox_stateC..."
        QT_MOC_LITERAL(112, 5)   // "state"
    },
    "AboutDialog",
    "checkUpdates",
    "",
    "updateCheckFinished",
    "on_checkUpdatesButton_clicked",
    "on_autoUpdatesCheckBox_stateChanged",
    "state"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_AboutDialog[] = {

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
       3,    0,   39,    2, 0x08,    2 /* Private */,
       4,    0,   40,    2, 0x08,    3 /* Private */,
       5,    1,   41,    2, 0x08,    4 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject AboutDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_AboutDialog.offsetsAndSizes,
    qt_meta_data_AboutDialog,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_AboutDialog_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AboutDialog, std::true_type>,
        // method 'checkUpdates'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateCheckFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_checkUpdatesButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_autoUpdatesCheckBox_stateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void AboutDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AboutDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->checkUpdates(); break;
        case 1: _t->updateCheckFinished(); break;
        case 2: _t->on_checkUpdatesButton_clicked(); break;
        case 3: _t->on_autoUpdatesCheckBox_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AboutDialog::*)();
            if (_t _q_method = &AboutDialog::checkUpdates; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *AboutDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AboutDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AboutDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int AboutDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void AboutDialog::checkUpdates()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
