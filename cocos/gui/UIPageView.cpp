/****************************************************************************
 Copyright (c) 2013 cocos2d-x.org
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "gui/UIPageView.h"

NS_CC_BEGIN

namespace gui {

PageView::PageView():
_curPageIdx(0),
_touchMoveDir(PAGEVIEW_TOUCHLEFT),
_touchStartLocation(0.0f),
_touchMoveStartLocation(0.0f),
_movePagePoint(Point::ZERO),
_leftChild(nullptr),
_rightChild(nullptr),
_leftBoundary(0.0f),
_rightBoundary(0.0f),
_isAutoScrolling(false),
_autoScrollDistance(0.0f),
_autoScrollSpeed(0.0f),
_autoScrollDir(0),
_childFocusCancelOffset(5.0f),
_pageViewEventListener(nullptr),
_pageViewEventSelector(nullptr)
{
}

PageView::~PageView()
{
    _pages.clear();
    _pageViewEventListener = nullptr;
    _pageViewEventSelector = nullptr;
}

PageView* PageView::create()
{
    PageView* widget = new PageView();
    if (widget && widget->init())
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return nullptr;
}

bool PageView::init()
{
    if (Layout::init())
    {
        setClippingEnabled(true);
        setUpdateEnabled(true);
        setTouchEnabled(true);
        return true;
    }
    return false;
}

void PageView::addWidgetToPage(Widget *widget, int pageIdx, bool forceCreate)
{
    if (!widget)
    {
        return;
    }
    if (pageIdx < 0)
    {
        return;
    }
    int pageCount = _pages.size();
    if (pageIdx < 0 || pageIdx >= pageCount)
    {
        if (forceCreate)
        {
            if (pageIdx > pageCount)
            {
                CCLOG("pageIdx is %d, it will be added as page id [%d]",pageIdx,pageCount);
            }
            Layout* newPage = createPage();
            newPage->addChild(widget);
            addPage(newPage);
        }
    }
    else
    {
        Layout * page = _pages.at(pageIdx);
        page->addChild(widget);
    }
}

Layout* PageView::createPage()
{
    Layout* newPage = Layout::create();
    newPage->setSize(getSize());
    return newPage;
}

void PageView::addPage(Layout* page)
{
    if (!page)
    {
        return;
    }
    if (page->getWidgetType() != WidgetTypeContainer)
    {
        return;
    }
    if (_pages.contains(page))
    {
        return;
    }
    Size pSize = page->getSize();
    Size pvSize = getSize();
    if (!pSize.equals(pvSize))
    {
        CCLOG("page size does not match pageview size, it will be force sized!");
        page->setSize(pvSize);
    }
    page->setPosition(Point(getPositionXByIndex(_pages.size()), 0));
    _pages.pushBack(page);
    addChild(page);
    updateBoundaryPages();
}

void PageView::insertPage(Layout* page, int idx)
{
    if (idx < 0)
    {
        return;
    }
    if (!page)
    {
        return;
    }
    if (page->getWidgetType() != WidgetTypeContainer)
    {
        return;
    }
    if (_pages.contains(page))
    {
        return;
    }
    
    int pageCount = _pages.size();
    if (idx >= pageCount)
    {
        addPage(page);
    }
    else
    {
        _pages.insert(idx, page);
        page->setPosition(Point(getPositionXByIndex(idx), 0));
        addChild(page);
        Size pSize = page->getSize();
        Size pvSize = getSize();
        if (!pSize.equals(pvSize))
        {
            CCLOG("page size does not match pageview size, it will be force sized!");
            page->setSize(pvSize);
        }
        int length = _pages.size();
        for (int i=(idx+1); i<length; i++) {
            Widget* behindPage = _pages.at(i);
            Point formerPos = behindPage->getPosition();
            behindPage->setPosition(Point(formerPos.x+getSize().width, 0));
        }
        updateBoundaryPages();
    }
}

void PageView::removePage(Layout* page)
{
    if (!page)
    {
        return;
    }
    removeChild(page);
    updateChildrenPosition();
    updateBoundaryPages();
}

void PageView::removePageAtIndex(int index)
{
    if (index < 0 || index >= (int)(_pages.size()))
    {
        return;
    }
    Layout* page = _pages.at(index);
    removePage(page);
}
    
void PageView::removeAllPages()
{
    removeAllChildren();
}

void PageView::updateBoundaryPages()
{
    if (_pages.size() <= 0)
    {
        _leftChild = nullptr;
        _rightChild = nullptr;
        return;
    }
    _leftChild = _pages.at(0);
    _rightChild = _pages.at(_pages.size()-1);
}

float PageView::getPositionXByIndex(int idx)
{
    return (getSize().width*(idx-_curPageIdx));
}
    
void PageView::addChild(Node *child)
{
    Layout::addChild(child);
}

void PageView::addChild(Node * child, int zOrder)
{
    Layout::addChild(child, zOrder);
}

void PageView::addChild(Node *child, int zOrder, int tag)
{
    Layout::addChild(child, zOrder, tag);
}

void PageView::removeChild(Node *child, bool cleanup)
{
    if (_pages.contains(static_cast<Layout*>(child)))
    {
        _pages.eraseObject(static_cast<Layout*>(child));
    }
    Layout::removeChild(child, cleanup);
}

void PageView::onSizeChanged()
{
    Layout::onSizeChanged();
    _rightBoundary = getSize().width;
    updateChildrenSize();
    updateChildrenPosition();
}

void PageView::updateChildrenSize()
{
    Size selfSize = getSize();
    int length = _pages.size();
    for (long i=0; i<length; i++)
    {
        Layout* page = _pages.at(i);
        page->setSize(selfSize);
    }
}

void PageView::updateChildrenPosition()
{
    int pageCount = _pages.size();
    if (pageCount <= 0)
    {
        _curPageIdx = 0;
        return;
    }
    if (_curPageIdx >= pageCount)
    {
        _curPageIdx = pageCount-1;
    }
    float pageWidth = getSize().width;
    for (int i=0; i<pageCount; i++)
    {
        Layout* page = _pages.at(i);
        page->setPosition(Point((i-_curPageIdx)*pageWidth, 0));
    }
}

void PageView::removeAllChildren()
{
    _pages.clear();
    Layout::removeAllChildren();
}

void PageView::scrollToPage(int idx)
{
    if (idx < 0 || idx >= (int)(_pages.size()))
    {
        return;
    }
    _curPageIdx = idx;
    Widget* curPage = _pages.at(idx);
    _autoScrollDistance = -(curPage->getPosition().x);
    _autoScrollSpeed = fabs(_autoScrollDistance)/0.2f;
    _autoScrollDir = _autoScrollDistance > 0 ? 1 : 0;
    _isAutoScrolling = true;
}

void PageView::update(float dt)
{
    if (_isAutoScrolling)
    {
        switch (_autoScrollDir)
        {
            case 0:
            {
                float step = _autoScrollSpeed*dt;
                if (_autoScrollDistance + step >= 0.0f)
                {
                    step = -_autoScrollDistance;
                    _autoScrollDistance = 0.0f;
                    _isAutoScrolling = false;
                }
                else
                {
                    _autoScrollDistance += step;
                }
                scrollPages(-step);
                if (!_isAutoScrolling)
                {
                    pageTurningEvent();
                }
                break;
            }
                break;
            case 1:
            {
                float step = _autoScrollSpeed*dt;
                if (_autoScrollDistance - step <= 0.0f)
                {
                    step = _autoScrollDistance;
                    _autoScrollDistance = 0.0f;
                    _isAutoScrolling = false;
                }
                else
                {
                    _autoScrollDistance -= step;
                }
                scrollPages(step);
                if (!_isAutoScrolling)
                {
                    pageTurningEvent();
                }
                break;
            }
            default:
                break;
        }
    }
}

bool PageView::onTouchBegan(Touch *touch, Event *unusedEvent)
{
    bool pass = Layout::onTouchBegan(touch, unusedEvent);
    if (_hitted)
    {
        handlePressLogic(touch->getLocation());
    }
    return pass;
}

void PageView::onTouchMoved(Touch *touch, Event *unusedEvent)
{
    _touchMovePos = touch->getLocation();
    handleMoveLogic(_touchMovePos);
    Widget* widgetParent = getWidgetParent();
    if (widgetParent)
    {
        widgetParent->checkChildInfo(1,this,_touchMovePos);
    }
    moveEvent();
    if (!hitTest(_touchMovePos))
    {
        setFocused(false);
        onTouchEnded(touch, unusedEvent);
    }
}

void PageView::onTouchEnded(Touch *touch, Event *unusedEvent)
{
    Layout::onTouchEnded(touch, unusedEvent);
    handleReleaseLogic(_touchEndPos);
}
    
void PageView::onTouchCancelled(Touch *touch, Event *unusedEvent)
{
    Layout::onTouchCancelled(touch, unusedEvent);
    handleReleaseLogic(touch->getLocation());
}

void PageView::movePages(float offset)
{
    int length = _pages.size();
    for (int i = 0; i < length; i++)
    {
        Widget* child = _pages.at(i);
        _movePagePoint.x = child->getPosition().x + offset;
        _movePagePoint.y = child->getPosition().y;
        child->setPosition(_movePagePoint);
    }
}

bool PageView::scrollPages(float touchOffset)
{
    if (_pages.size() <= 0)
    {
        return false;
    }
    
    if (!_leftChild || !_rightChild)
    {
        return false;
    }
    
    float realOffset = touchOffset;
    
    switch (_touchMoveDir)
    {
        case PAGEVIEW_TOUCHLEFT: // left
            if (_rightChild->getRightInParent() + touchOffset <= _rightBoundary)
            {
                realOffset = _rightBoundary - _rightChild->getRightInParent();
                movePages(realOffset);
                return false;
            }
            break;
            
        case PAGEVIEW_TOUCHRIGHT: // right
            if (_leftChild->getLeftInParent() + touchOffset >= _leftBoundary)
            {
                realOffset = _leftBoundary - _leftChild->getLeftInParent();
                movePages(realOffset);
                return false;
            }
            break;
        default:
            break;
    }
    
    movePages(realOffset);
    return true;
}

void PageView::handlePressLogic(const Point &touchPoint)
{
    Point nsp = convertToNodeSpace(touchPoint);
    _touchMoveStartLocation = nsp.x;
    _touchStartLocation = nsp.x;
}

void PageView::handleMoveLogic(const Point &touchPoint)
{
    Point nsp = convertToNodeSpace(touchPoint);
    float offset = 0.0;
    float moveX = nsp.x;
    offset = moveX - _touchMoveStartLocation;
    _touchMoveStartLocation = moveX;
    if (offset < 0)
    {
        _touchMoveDir = PAGEVIEW_TOUCHLEFT;
    }
    else if (offset > 0)
    {
        _touchMoveDir = PAGEVIEW_TOUCHRIGHT;
    }
    scrollPages(offset);
}

void PageView::handleReleaseLogic(const Point &touchPoint)
{
    if (_pages.size() <= 0)
    {
        return;
    }
    Widget* curPage = _pages.at(_curPageIdx);
    if (curPage)
    {
        Point curPagePos = curPage->getPosition();
        int pageCount = _pages.size();
        float curPageLocation = curPagePos.x;
        float pageWidth = getSize().width;
        float boundary = pageWidth/2.0f;
        if (curPageLocation <= -boundary)
        {
            if (_curPageIdx >= pageCount-1)
            {
                scrollPages(-curPageLocation);
            }
            else
            {
                scrollToPage(_curPageIdx+1);
            }
        }
        else if (curPageLocation >= boundary)
        {
            if (_curPageIdx <= 0)
            {
                scrollPages(-curPageLocation);
            }
            else
            {
                scrollToPage(_curPageIdx-1);
            }
        }
        else
        {
            scrollToPage(_curPageIdx);
        }
    }
}

void PageView::checkChildInfo(int handleState,Widget* sender, const Point &touchPoint)
{
    interceptTouchEvent(handleState, sender, touchPoint);
}

void PageView::interceptTouchEvent(int handleState, Widget *sender, const Point &touchPoint)
{
    switch (handleState)
    {
        case 0:
            handlePressLogic(touchPoint);
            break;
        case 1:
        {
            float offset = 0;
            offset = fabs(sender->getTouchStartPos().x - touchPoint.x);
            if (offset > _childFocusCancelOffset)
            {
                sender->setFocused(false);
                handleMoveLogic(touchPoint);
            }
        }
            break;
        case 2:
            handleReleaseLogic(touchPoint);
            break;
            
        case 3:
            break;
    }
}

void PageView::pageTurningEvent()
{
    if (_pageViewEventListener && _pageViewEventSelector)
    {
        (_pageViewEventListener->*_pageViewEventSelector)(this, PAGEVIEW_EVENT_TURNING);
    }
}

void PageView::addEventListenerPageView(Object *target, SEL_PageViewEvent selector)
{
    _pageViewEventListener = target;
    _pageViewEventSelector = selector;
}

int PageView::getCurPageIndex() const
{
    return _curPageIdx;
}

Vector<Layout*>& PageView::getPages()
{
    return _pages;
}
    
Layout* PageView::getPage(int index)
{
    if (index < 0 || index >= (int)(_pages.size()))
    {
        return nullptr;
    }
    return _pages.at(index);
}

std::string PageView::getDescription() const
{
    return "PageView";
}

Widget* PageView::createCloneInstance()
{
    return PageView::create();
}

void PageView::copyClonedWidgetChildren(Widget* model)
{
    Vector<Layout*> modelPages = dynamic_cast<PageView*>(model)->getPages();
    int length = modelPages.size();
    for (int i=0; i<length; i++)
    {
        Layout* page = modelPages.at(i);
        addPage(dynamic_cast<Layout*>(page->clone()));
    }
}

void PageView::copySpecialProperties(Widget *widget)
{
    PageView* pageView = dynamic_cast<PageView*>(widget);
    if (pageView)
    {
        Layout::copySpecialProperties(widget);
    }
}

}

NS_CC_END