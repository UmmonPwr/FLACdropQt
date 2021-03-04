#include "FLACdropQt.h"

void FLACdropQt::createLogWindow()
{
	logWindow = new QWidget;
	log_layout = new QGridLayout;
	log_textbox = new QTextEdit;

	log_textbox->setLineWrapMode(QTextEdit::NoWrap);
	log_textbox->setReadOnly(true);

	log_layout->addWidget(log_textbox);
	logWindow->setLayout(log_layout);

	logWindow->setWindowTitle(tr("Log history"));
	logWindow->setMinimumSize(460, 360);
	logWindow->resize(460, 360);
	logWindow->setWindowIcon(*icon_FLACdrop);
}

void FLACdropQt::dialog_log()
{
	main_label_info->setText(tr("Invoked <b>Log|Log Dialog</b>"));
	logWindow->show();
}

void FLACdropQt::addLogResult(QString file, int result)
{
	log_textbox->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);	// move the cursor to the end, user can move the cursor in the QTextEdit even if it is set to read only and cursor is not visible
	
	log_textbox->insertPlainText(file);
	log_textbox->insertPlainText("\n");
	//log_textbox->insertPlainText(QString::number(result));
	log_textbox->insertPlainText(FAIL_TEXT[result]);
	log_textbox->insertPlainText("\n\n");
}