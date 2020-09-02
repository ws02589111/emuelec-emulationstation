#include "guis/GuiInfoPopup.h"

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include <SDL_timer.h>

GuiInfoPopup::GuiInfoPopup(Window* window, std::string message, int duration) :
	GuiComponent(window), mMessage(message), mDuration(duration), mRunning(true), mFadeOut(0)
{
	auto theme = ThemeData::getMenuTheme();
	mBackColor = theme->Background.color;

	mFrame = new NinePatchComponent(window);
	float maxWidth = Renderer::getScreenWidth() * 0.9f;
	float maxHeight = Renderer::getScreenHeight() * 0.2f;

	std::shared_ptr<TextComponent> s = std::make_shared<TextComponent>(mWindow,
		"",
		Font::get(FONT_SIZE_MINI),
		theme->Text.color,
		ALIGN_CENTER);

	// we do this to force the text container to resize and return an actual expected popup size
	s->setSize(0,0);
	s->setText(message);
	mSize = s->getSize();

	// confirm the size isn't larger than the screen width, otherwise cap it
	if (mSize.x() > maxWidth) {
		s->setSize(maxWidth, mSize[1]);
		mSize[0] = maxWidth;
	}
	if (mSize.y() > maxHeight) {
		s->setSize(mSize[0], maxHeight);
		mSize[1] = maxHeight;
	}

	// add a padding to the box
	int paddingX = (int) (Renderer::getScreenWidth() * 0.03f);
	int paddingY = (int) (Renderer::getScreenHeight() * 0.02f);
	mSize[0] = mSize.x() + paddingX;
	mSize[1] = mSize.y() + paddingY;

	float posX = Renderer::getScreenWidth()*0.5f - mSize.x()*0.5f;
	float posY = Renderer::getScreenHeight() * 0.02f;

	setPosition(posX, posY, 0);

	mFrame->setImagePath(theme->Background.path);
	mFrame->setCenterColor(mBackColor);
	mFrame->setEdgeColor(mBackColor);
	mFrame->setCornerSize(theme->Background.cornerSize);
	mFrame->fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	addChild(mFrame);

	// we only init the actual time when we first start to render
	mStartTime = 0;

	mGrid = new ComponentGrid(window, Vector2i(1, 3));
	mGrid->setSize(mSize);
	mGrid->setEntry(s, Vector2i(0, 1), false, true);
	addChild(mGrid);
}

void GuiInfoPopup::render(const Transform4x4f& /*parentTrans*/)
{
	if (!mRunning)
		return;

	// we use identity as we want to render on a specific window position, not on the view
	Transform4x4f trans = getTransform() * Transform4x4f::Identity();

	Renderer::setMatrix(trans);
	renderChildren(trans);	
}

void GuiInfoPopup::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	int curTime = SDL_GetTicks();

	// we only init the actual time when we first start to render
	if (mStartTime == 0)
		mStartTime = curTime;
	else if (curTime - mStartTime > mDuration || curTime < mStartTime)
	{
		// If we're past the popup duration, no need to render or if SDL reset : Stop
		mRunning = false;
		mFadeOut = 1.0;
		return;
	}

	// compute fade in effect
	int alpha = 255;

	if (curTime - mStartTime <= 500)
		alpha = ((curTime - mStartTime)*255/500);
	else if (curTime - mStartTime >= mDuration - 500)
	{
		alpha = ((-(curTime - mStartTime - mDuration) * 255) / 500);
		mFadeOut = (float) (255 - alpha) / 255.0;
	}

	if (alpha > mBackColor & 0xff)
		alpha = mBackColor & 0xff;

	mGrid->setOpacity((unsigned char)alpha);

	// apply fade in effect to popup frame
	mFrame->setEdgeColor((mBackColor & 0xffffff00) | (unsigned char)(alpha));
	mFrame->setCenterColor((mBackColor & 0xffffff00) | (unsigned char)(alpha));
}
