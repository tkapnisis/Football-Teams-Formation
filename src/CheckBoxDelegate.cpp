#include "CheckBoxDelegate.h"

CheckBoxDelegate::CheckBoxDelegate(QObject *parent ):QItemDelegate(parent)
{
}

void CheckBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	//drawCheck(painter,option,option.rect,index.model()->data(index,Qt::DisplayRole).toBool()?Qt::Checked:Qt::Unchecked);
	drawDisplay(painter,option,option.rect,index.model()->data( index, Qt::DisplayRole ).toBool()?QString("      ").append(tr("Yes")):QString("      ").append(tr("No")));
	drawFocus(painter,option,option.rect);
}

QWidget *CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	(void)option;
	(void)index;
	theCheckBox = new QCheckBox( parent );
	QObject::connect(theCheckBox,SIGNAL(toggled(bool)),this,SLOT(setData(bool)));
	return theCheckBox;
}

void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	bool val = index.model()->data( index, Qt::DisplayRole ).toBool();
	(static_cast<QCheckBox*>( editor ))->setChecked(val);
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	model->setData( index, static_cast<bool>(static_cast<QCheckBox*>( editor )->isChecked() ) );
}


void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	(void)index;
	(void)option;
	editor->setGeometry( option.rect );
}

void CheckBoxDelegate::setData(bool val)
{
	(void)val;
	emit commitData(theCheckBox);
}
