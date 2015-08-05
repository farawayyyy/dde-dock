#include "docklayout.h"
#include "abstractdockitem.h"

DockLayout::DockLayout(QWidget *parent) :
    QWidget(parent)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
}

void DockLayout::addItem(AbstractDockItem *item)
{
    if (m_lastHoverIndex == -1)
        insertItem(item,m_appList.count());
    else
        insertItem(item, m_lastHoverIndex);
}

void DockLayout::insertItem(AbstractDockItem *item, int index)
{
    item->setParent(this);
    item->show();
    int appCount = m_appList.count();
    index = index > appCount ? appCount : (index < 0 ? 0 : index);

    m_appList.insert(index,item);

    connect(item, &AbstractDockItem::frameUpdate, this, &DockLayout::frameUpdate);
    connect(item, &AbstractDockItem::posChanged, this, &DockLayout::frameUpdate);
    connect(item, &AbstractDockItem::mouseRelease, this, &DockLayout::slotItemRelease);
    connect(item, &AbstractDockItem::dragStart, this, &DockLayout::slotItemDrag);
    connect(item, &AbstractDockItem::dragEntered, this, &DockLayout::slotItemEntered);
    connect(item, &AbstractDockItem::dragExited, this, &DockLayout::slotItemExited);
    connect(item, &AbstractDockItem::widthChanged, this, &DockLayout::relayout);
    connect(item, &AbstractDockItem::moveAnimationFinished,this, &DockLayout::slotAnimationFinish);

    m_ddam->Sort(itemsIdList());

    relayout();
}

void DockLayout::removeItem(int index)
{
    delete m_appList.takeAt(index);
    relayout();
}

void DockLayout::setSpacing(qreal spacing)
{
    this->m_itemSpacing = spacing;
}

void DockLayout::setVerticalAlignment(Qt::Alignment value)
{
    this->m_verticalAlignment = value;
}

void DockLayout::setSortDirection(DockLayout::Direction value)
{
    this->m_sortDirection = value;
}

void DockLayout::sortLeftToRight()
{
    if (m_appList.count() <= 0)
        return;

    for (int i = 0; i < m_appList.count(); i ++)
    {
        AbstractDockItem * toItem = m_appList.at(i);

        int nextX = 0;
        int nextY = 0;
        if (i > 0){
            AbstractDockItem * frontItem = m_appList.at(i - 1);
            nextX = frontItem->x() + frontItem->width() + m_itemSpacing;
        }
        else
            nextX = m_itemSpacing;

        switch (m_verticalAlignment)
        {
        case Qt::AlignTop:
            nextY = 0;
            break;
        case Qt::AlignVCenter:
            nextY = (height() - toItem->height()) / 2;
            break;
        case Qt::AlignBottom:
            nextY = height() - toItem->height();
            break;
        }

        toItem->move(QPoint(nextX, nextY));
        toItem->setNextPos(toItem->pos());
    }
}

void DockLayout::sortRightToLeft()
{
    if (m_appList.count()<=0)
        return;

    for (int i = 0; i < m_appList.count(); i++)
    {
        AbstractDockItem *toItem = m_appList.at(i);

        int nextX = 0;
        int nextY = 0;
        if (i > 0){
            AbstractDockItem *frontItem = m_appList.at(i - 1);
            nextX = frontItem->x() - m_itemSpacing - toItem->width();
        }
        else
            nextX = getContentsWidth() - m_itemSpacing - m_appList.first()->width();

        switch (m_verticalAlignment)
        {
        case Qt::AlignTop:
            nextY = 0;
            break;
        case Qt::AlignVCenter:
            nextY = (height() - toItem->height()) / 2;
            break;
        case Qt::AlignBottom:
            nextY = height() - toItem->height();
            break;
        }

        toItem->move(QPoint(nextX, nextY));
        toItem->setNextPos(toItem->pos());
    }
}

int DockLayout::spacingItemWidth()
{
    if (m_dragItemMap.isEmpty())
        return DockModeData::instance()->getNormalItemWidth();
    else
        return m_dragItemMap.firstKey()->width();
}

int DockLayout::spacingItemIndex()
{
    if (m_sortDirection == RightToLeft)
        return -1;
    if (m_appList.count() <= 1)
        return -1;
    if (m_appList.at(0)->getNextPos().x() > m_itemSpacing)
        return 0;

    for (int i = 1; i < m_appList.count(); i ++)
    {
        if (m_appList.at(i)->getNextPos().x() - m_itemSpacing != m_appList.at(i - 1)->getNextPos().x() + m_appList.at(i - 1)->width())
            return i;
    }
    return -1;
}

QStringList DockLayout::itemsIdList()
{
    QStringList dockedAppList = m_ddam->DockedAppList().value();

    QStringList idList;
    foreach (AbstractDockItem *item, m_appList) {
        QString itemId = item->getItemId();
        if (!itemId.isEmpty() && dockedAppList.indexOf(itemId) != -1)
            idList << itemId;
    }
    return idList;
}

void DockLayout::leftToRightMove(int hoverIndex)
{
    int itemWidth = spacingItemWidth();
    int spacintIndex = spacingItemIndex();
    if (spacintIndex == -1)
        return;

    if (spacintIndex > hoverIndex)
    {
        for (int i = hoverIndex; i < spacintIndex; i ++)
        {
            AbstractDockItem *targetItem = m_appList.at(i);
            QPoint nextPos = QPoint(targetItem->x() + itemWidth + m_itemSpacing,0);
            if (targetItem->x() != targetItem->getNextPos().x())    //animation not finish
                break;
            m_animationItemCount ++;
            targetItem->moveWithAnimation(nextPos, MOVE_ANIMATION_DURATION_BASE + m_animationItemCount * 25);
        }
    }
    else
    {
        for (int i = spacintIndex; i <= hoverIndex; i ++)
        {
            AbstractDockItem *targetItem = m_appList.at(i);
            QPoint nextPos = QPoint(targetItem->x() - itemWidth - m_itemSpacing,0);
            if (targetItem->x() != targetItem->getNextPos().x())    //animation not finish
                break;
            m_animationItemCount ++;
            targetItem->moveWithAnimation(nextPos, MOVE_ANIMATION_DURATION_BASE + m_animationItemCount * 25);
        }
    }
}

void DockLayout::rightToLeftMove(int hoverIndex)
{

}

int DockLayout::indexOf(AbstractDockItem *item)
{
    return m_appList.indexOf(item);
}

int DockLayout::indexOf(int x, int y)
{
    for (int i = 0; i < m_appList.count(); i ++) {
        if (m_appList.at(i)->geometry().contains(x, y))
            return i;
    }

    return -1;
}

void DockLayout::restoreTmpItem()
{
    if (m_dragItemMap.isEmpty())
        return;

    AbstractDockItem * tmpItem = m_dragItemMap.firstKey();
    m_dragItemMap.remove(tmpItem);
    tmpItem->setVisible(true);
    if (indexOf(tmpItem) == -1)
    {
        if (m_movingForward)
            insertItem(tmpItem,m_lastHoverIndex);
        else
            insertItem(tmpItem,m_lastHoverIndex + 1);
    }

    emit itemDropped();
    m_animationItemCount = 0;
}

void DockLayout::relayout()
{
    switch (m_sortDirection)
    {
    case LeftToRight:
        sortLeftToRight();
        break;
    case RightToLeft:
        sortRightToLeft();
        break;
    default:
        break;
    }

    emit contentsWidthChange();
}

void DockLayout::
clearTmpItem()
{
    if (m_dragItemMap.count() > 0)
    {
        AbstractDockItem * tmpItem = m_dragItemMap.firstKey();
        m_dragItemMap.clear();
        tmpItem->deleteLater();
    }
}

void DockLayout::addSpacingItem()
{
    int spacingValue = 0;
    if (m_dragItemMap.isEmpty())
        spacingValue = DockModeData::instance()->getNormalItemWidth();
    else
    {
        AbstractDockItem *tmpItem = m_dragItemMap.firstKey();
        spacingValue = tmpItem->width();
    }

    for (int i = m_appList.count() -1;i >= m_lastHoverIndex; i-- )
    {
        AbstractDockItem *targetItem = m_appList.at(i);
        targetItem->setNextPos(targetItem->x() + spacingValue + m_itemSpacing,0);

        QPropertyAnimation *animation = new QPropertyAnimation(targetItem, "pos");
        animation->setStartValue(targetItem->pos());
        animation->setEndValue(targetItem->getNextPos());
        animation->setDuration(150 + i * 10);
        animation->setEasingCurve(QEasingCurve::OutCubic);

        animation->start();
        connect(animation, SIGNAL(finished()),this, SIGNAL(contentsWidthChange()));
    }

    emit contentsWidthChange();
}

void DockLayout::dragoutFromLayout(int index)
{
    AbstractDockItem * tmpItem = m_appList.takeAt(index);
    tmpItem->setVisible(false);
    m_dragItemMap.insert(tmpItem,index);

    emit contentsWidthChange();
}

int DockLayout::getContentsWidth()
{
    int tmpWidth = m_appList.count() * m_itemSpacing;
    for (int i = 0; i < m_appList.count(); i ++)
        tmpWidth += m_appList.at(i)->width();

    if (spacingItemIndex() != -1 && !m_dragItemMap.isEmpty() && m_dragItemMap.firstKey())
        tmpWidth += m_dragItemMap.firstKey()->width() + m_itemSpacing;

    return tmpWidth;
}

int DockLayout::getItemCount()
{
    return m_appList.count();
}

QList<AbstractDockItem *> DockLayout::getItemList() const
{
    return m_appList;
}

void DockLayout::dragEnterEvent(QDragEnterEvent *event)
{
    event->setDropAction(Qt::MoveAction);
    event->accept();
}

void DockLayout::dropEvent(QDropEvent *event)
{
    AbstractDockItem *sourceItem = dynamic_cast<AbstractDockItem *>(event->source());

    if (!sourceItem && event->mimeData()->formats().indexOf("_DEEPIN_DND") != -1)
    {
        QString jsonStr = QString(event->mimeData()->data("_DEEPIN_DND")).split("uninstall").last().trimmed();

        //Tim at both ends of the string is added to a character SOH (start of heading)
        jsonStr = jsonStr.mid(1,jsonStr.length() - 2);
        QJsonObject dataObj = QJsonDocument::fromJson(jsonStr.trimmed().toUtf8()).object();
        if (dataObj.isEmpty() || m_ddam->IsDocked(dataObj.value("id").toString()))
        {
            relayout();
            return;
        }

        m_ddam->ReqeustDock(dataObj.value("id").toString(),dataObj.value("name").toString(),dataObj.value("icon").toString(),"");

    }
    else if (sourceItem && event->mimeData()->formats().indexOf("_DEEPIN_DND") == -1)
        restoreTmpItem();
}

void DockLayout::slotItemDrag()
{
//    qWarning() << "Item draging..."<<x<<y<<item;
    AbstractDockItem *item = qobject_cast<AbstractDockItem*>(sender());

    int tmpIndex = indexOf(item);
    if (tmpIndex != -1)
    {
        m_lastHoverIndex = tmpIndex;
        m_lastPost = QCursor::pos();
        dragoutFromLayout(tmpIndex);

        emit dragStarted();
    }
}

void DockLayout::slotItemRelease()
{
    //outside frame,destroy it
    //inside frame,insert it
    AbstractDockItem *item = qobject_cast<AbstractDockItem*>(sender());

    item->setVisible(true);
    if (indexOf(item) == -1)
        insertItem(item,m_lastHoverIndex);
}

void DockLayout::slotItemEntered(QDragEnterEvent *)
{
    AbstractDockItem *item = qobject_cast<AbstractDockItem*>(sender());

    int tmpIndex = indexOf(item);
    m_lastHoverIndex = tmpIndex;
    if (spacingItemIndex() == -1 && m_animationItemCount <= 0){  //if some animation still running ,there must has spacing item
        addSpacingItem();
        return;
    }

    QPoint tmpPos = QCursor::pos();

    if (tmpPos.x() - m_lastPost.x() == 0)
        return;

    bool lastState = m_movingForward;
    switch (m_sortDirection)
    {
    case LeftToRight:
        m_movingForward = tmpPos.x() - m_lastPost.x() < 0;
        if (m_movingForward != lastState && m_animationItemCount > 0)
        {
            m_movingForward = lastState;
            return;
        }
        leftToRightMove(tmpIndex);
        break;
    case RightToLeft:
        m_movingForward = tmpPos.x() - m_lastPost.x() > 0;
        if (m_movingForward != lastState && m_animationItemCount > 0)
        {
            m_movingForward = lastState;
            return;
        }
        rightToLeftMove(tmpIndex);
        break;
    }

    m_lastPost = tmpPos;

}

void DockLayout::slotItemExited(QDragLeaveEvent *)
{

}

void DockLayout::slotAnimationFinish()
{
    if (m_animationItemCount > 0){
        //now the animation count should be 0
        //for overlap
        //e.g: spacingIndex is 4 and now if drag item hover item(1) and out of dock suddenly
        //item(1~3) will move to index 4 witch is no longer a spacingItem
        if (m_animationItemCount == 1 && spacingItemIndex() == -1)
            relayout();

        m_animationItemCount --;
    }
}
