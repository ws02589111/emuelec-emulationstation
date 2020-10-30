#include "components/ButtonComponent.h"

#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "LocaleES.h"

ButtonComponent::ButtonComponent(Window* window, const std::string& text, const std::string& helpText, const std::function<void()>& func, bool upperCase) : GuiComponent(window),
	mBox(window, ThemeData::getMenuTheme()->Icons.button),	
	mFocused(false), 
	mEnabled(true),
	mPadding(Vector4f(0, 0, 0, 0))
{
	auto menuTheme = ThemeData::getMenuTheme();

	mFont = menuTheme->Text.font;
	mTextColorUnfocused = menuTheme->Text.color;
	mTextColorFocused = menuTheme->Text.selectedColor;
	mColor = menuTheme->Text.color;
	mColorFocused = menuTheme->Text.selectorColor;
	mRenderNonFocusedBackground = true;
	
	if (Renderer::isSmallScreen())
		mBox.setCornerSize(8, 8);

	setPressedFunc(func);
	setText(text, helpText, upperCase);
	updateImage();
}

void ButtonComponent::onSizeChanged()
{
	auto sz = mBox.getCornerSize();

	mBox.fitTo(
		Vector2f(mSize.x() - mPadding.x() - mPadding.z(), mSize.y() - mPadding.y() - mPadding.w()), 
		Vector3f(mPadding.x(), mPadding.y()), 
		Vector2f(-sz.x() * 2, -sz.y() * 2));
}

void ButtonComponent::setPressedFunc(std::function<void()> f)
{
	mPressedFunc = f;
}

bool ButtonComponent::input(InputConfig* config, Input input)
{
	if(config->isMappedTo(BUTTON_OK, input) && input.value != 0)
	{
		if(mPressedFunc && mEnabled)
			mPressedFunc();
		return true;
	}

	return GuiComponent::input(config, input);
}

void ButtonComponent::setText(const std::string& text, const std::string& helpText, bool upperCase)
{
	mText = upperCase ? Utils::String::toUpper(text) : text;
	mHelpText = helpText;
	
	mTextCache = std::unique_ptr<TextCache>(mFont->buildTextCache(mText, 0, 0, getCurTextColor()));

	float padding = Renderer::isSmallScreen() ? 12 : 24;

	float minWidth = mFont->sizeText("DELETE").x() + padding;
	setSize(Math::max(mTextCache->metrics.size.x() + padding, minWidth), mTextCache->metrics.size.y());

	updateHelpPrompts();
}

void ButtonComponent::onFocusGained()
{
	mFocused = true;
	updateImage();
}

void ButtonComponent::onFocusLost()
{
	mFocused = false;
	updateImage();
}

void ButtonComponent::setEnabled(bool enabled)
{
	mEnabled = enabled;
	updateImage();
}

void ButtonComponent::updateImage()
{
	if(!mEnabled || !mPressedFunc)
	{
		mBox.setImagePath(":/button_filled.png");
		mBox.setCenterColor(0x770000FF);
		mBox.setEdgeColor(0x770000FF);
		return;
	}

        // batocera
	// If a new color has been set.  
	if (mNewColor) {
		mBox.setImagePath(ThemeData::getMenuTheme()->Icons.button_filled);
		mBox.setCenterColor(mModdedColor);
		mBox.setEdgeColor(mModdedColor);
		return;
	}

	mBox.setCenterColor(getCurBackColor());
	mBox.setEdgeColor(getCurBackColor());
	mBox.setImagePath(mFocused ? ThemeData::getMenuTheme()->Icons.button_filled : ThemeData::getMenuTheme()->Icons.button);
}

void ButtonComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	if (mRenderNonFocusedBackground || mFocused)
		mBox.render(trans);
	else
	{
		Renderer::setMatrix(trans);
		Renderer::drawRect(mPadding.x(), mPadding.y(), mSize.x() - mPadding.x() - mPadding.z(), mSize.y() - mPadding.y() - mPadding.w(), 0x60606025);
	}

	if(mTextCache)
	{
		Vector3f centerOffset((mSize.x() - mTextCache->metrics.size.x()) / 2, (mSize.y() - mTextCache->metrics.size.y()) / 2, 0);
		trans = trans.translate(centerOffset);

		Renderer::setMatrix(trans);
		mTextCache->setColor(getCurTextColor());
		mFont->renderTextCache(mTextCache.get());
		trans = trans.translate(-centerOffset);
	}

	renderChildren(trans);
}

unsigned int ButtonComponent::getCurTextColor() const
{
	if(!mFocused)
		return mTextColorUnfocused;
	else
		return mTextColorFocused;
}

unsigned int ButtonComponent::getCurBackColor() const
{
	if (!mFocused)
		return mColor;
	else
		return mColorFocused;
}

std::vector<HelpPrompt> ButtonComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_OK, mHelpText.empty() ? mText.c_str() : mHelpText.c_str())); // batocera
	return prompts;
}


void ButtonComponent::setPadding(const Vector4f padding)
{
	if (mPadding == padding)
		return;

	mPadding = padding;
	onSizeChanged();
}
