#include "FLACdropQt.h"

void FLACdropQt::dialog_about()
{
	main_label_info->setText(tr("Invoked <b>Help|About</b>"));
	QMessageBox::about(this, tr("About"), tr("<b>FLACdropQt v0.3</b><br /><br />Using libraries:<br />- libFLAC 1.3.3 <i>- GitHub version</i><br />- libOGG 1.3.3 <i>- GitHub version</i><br />- libMP3lame 3.100.2"));
}

void FLACdropQt::dialog_aboutQt()
{
	main_label_info->setText(tr("Invoked <b>Help|About Qt</b>"));
}