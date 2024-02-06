#pragma once
#ifndef QFileDialogDefaultOptions_HPHPHPH
#define QFileDialogDefaultOptions_HPHPHPH

#ifdef QT_DONT_USE_NATIVE_FILE_DIALOG
const QFileDialog::Options c_fileDialogOptions = QFileDialog::DontUseNativeDialog|QFileDialog::DontUseCustomDirectoryIcons;
#else
const QFileDialog::Options c_fileDialogOptions = QFileDialog::Options();
#endif

#ifdef QT_DONT_USE_NATIVE_FILE_DIALOG
const QFileDialog::Options c_dirDialogOptions = QFileDialog::ShowDirsOnly|QFileDialog::DontUseNativeDialog|QFileDialog::DontUseCustomDirectoryIcons;
#else
const QFileDialog::Options c_dirDialogOptions = QFileDialog::ShowDirsOnly;
#endif

#endif // QFileDialogDefaultOptions_HPHPHPH
