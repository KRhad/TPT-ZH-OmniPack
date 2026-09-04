#include "ServerSaveActivity.h"
#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/SaveIDMessage.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "client/Client.h"
#include "client/ThumbnailRendererTask.h"
#include "client/GameSave.h"
#include "client/http/UploadSaveRequest.h"
#include "tasks/Task.h"
#include "gui/Style.h"

class SaveUploadTask: public Task
{
	SaveInfo &save;

	void before() override
	{

	}

	void after() override
	{

	}

	bool doWork() override
	{
		notifyProgress(-1);
		auto uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(save);
		uploadSaveRequest->Start();
		uploadSaveRequest->Wait();
		try
		{
			save.SetID(uploadSaveRequest->Finish());
		}
		catch (const http::RequestError &ex)
		{
			notifyError(ByteString(ex.what()).FromUtf8());
			return false;
		}
		return true;
	}

public:
	SaveUploadTask(SaveInfo &newSave):
		save(newSave)
	{

	}
};

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(440, 200)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	titleLabel = new ui::Label(ui::Point(4, 5), ui::Point((Size.X/2)-8, 16), "");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);
	CheckName(save->GetName()); //set titleLabel text

	ui::Label * previewLabel = new ui::Label(ui::Point((Size.X/2)+4, 5), ui::Point((Size.X/2)-8, 16), "预览：");
	previewLabel->SetTextColour(style::Colour::InformationTitle);
	previewLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	previewLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(previewLabel);

	nameField = new ui::Textbox(ui::Point(8, 25), ui::Point((Size.X/2)-16, 16), save->GetName(), "[存档名称]");
	nameField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	nameField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	nameField->SetActionCallback({ [this] { CheckName(nameField->GetText()); } });
	nameField->SetLimit(50);
	AddComponent(nameField);
	FocusComponent(nameField);

	descriptionField = new ui::Textbox(ui::Point(8, 65), ui::Point((Size.X/2)-16, Size.Y-(65+16+4)), save->GetDescription(), "[存档说明]");
	descriptionField->SetMultiline(true);
	descriptionField->SetLimit(254);
	descriptionField->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	descriptionField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(descriptionField);

	publishedCheckbox = new ui::Checkbox(ui::Point(8, 45), ui::Point((Size.X/2)-80, 16), "发布", "");
	auto user = Client::Ref().GetAuthUser();
	if (!(user && user->Username == save->GetUserName()))
	{
		//Save is not owned by the user, disable by default
		publishedCheckbox->SetChecked(false);
	}
	else
	{
		//Save belongs to the current user, use published state already set
		publishedCheckbox->SetChecked(save->GetPublished());
	}
	AddComponent(publishedCheckbox);

	pausedCheckbox = new ui::Checkbox(ui::Point(160, 45), ui::Point(55, 16), "已暂停", "");
	pausedCheckbox->SetChecked(save->GetGameSave()->paused);
	AddComponent(pausedCheckbox);

	ui::Button * cancelButton = new ui::Button(ui::Point(0, Size.Y-16), ui::Point((Size.X/2)-75, 16), "取消");
	cancelButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	cancelButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	cancelButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	cancelButton->SetActionCallback({ [this] {
		Exit();
	} });
	AddComponent(cancelButton);
	SetCancelButton(cancelButton);

	ui::Button * okayButton = new ui::Button(ui::Point((Size.X/2)-76, Size.Y-16), ui::Point(76, 16), "保存");
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.TextInactive = style::Colour::InformationTitle;
	okayButton->SetActionCallback({ [this] {
		Save();
	} });
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	ui::Button * PublishingInfoButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-42), ui::Point(150, 16), "发布说明");
	PublishingInfoButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	PublishingInfoButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	PublishingInfoButton->Appearance.TextInactive = style::Colour::InformationTitle;
	PublishingInfoButton->SetActionCallback({ [this] {
		ShowPublishingInfo();
	} });
	AddComponent(PublishingInfoButton);

	ui::Button * RulesButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-22), ui::Point(150, 16), "存档上传规则");
	RulesButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	RulesButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	RulesButton->Appearance.TextInactive = style::Colour::InformationTitle;
	RulesButton->SetActionCallback({ [this] {
		ShowRules();
	} });
	AddComponent(RulesButton);

	if (save->GetGameSave())
	{
		thumbnailRenderer = new ThumbnailRendererTask(*save->GetGameSave(), Size / 2 - Vec2(16, 16), RendererSettings::decorationAntiClickbait, true);
		thumbnailRenderer->Start();
	}
}

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, bool saveNow, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(200, 50)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	ui::Label * titleLabel = new ui::Label(ui::Point(0, 0), Size, "正在保存到服务器……");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);

	AddAuthorInfo();

	saveUploadTask = new SaveUploadTask(*this->save);
	saveUploadTask->AddTaskListener(this);
	saveUploadTask->Start();
}

void ServerSaveActivity::NotifyDone(Task * task)
{
	if(!task->GetSuccess())
	{
		Exit();
		new ErrorMessage("错误", task->GetError());
	}
	else
	{
		if (onUploaded)
		{
			onUploaded(std::move(save));
		}
		Exit();
	}
}

void ServerSaveActivity::Save()
{
	if (!nameField->GetText().length())
	{
		new ErrorMessage("错误", "必须填写存档名称。");
		return;
	}
	auto user = Client::Ref().GetAuthUser();
	if (!(user && user->Username == save->GetUserName()) && publishedCheckbox->GetChecked())
	{
		new ConfirmPrompt("发布", "此存档的作者是 " + save->GetUserName().FromUtf8() + "；你即将以自己的名义发布它。若未获得原作者许可，请取消勾选“发布”；否则可继续。", { [this] {
			saveUpload();
		} });
	}
	else
	{
		saveUpload();
	}
}

void ServerSaveActivity::AddAuthorInfo()
{
	Bson serverSaveInfo;
	serverSaveInfo["type"] = "save";
	serverSaveInfo["id"] = save->GetID();
	auto user = Client::Ref().GetAuthUser();
	serverSaveInfo["username"] = user ? user->Username : ByteString("");
	serverSaveInfo["title"] = save->GetName().ToUtf8();
	serverSaveInfo["description"] = save->GetDescription().ToUtf8();
	serverSaveInfo["published"] = (int)save->GetPublished();
	serverSaveInfo["date"] = int64_t(time(nullptr));
	Client::Ref().SaveAuthorInfo(serverSaveInfo);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->authors = serverSaveInfo;
		save->SetGameSave(std::move(gameSave));
	}
}

void ServerSaveActivity::saveUpload()
{
	okayButton->Enabled = false;
	save->SetName(nameField->GetText());
	save->SetDescription(descriptionField->GetText());
	save->SetPublished(publishedCheckbox->GetChecked());
	auto user = Client::Ref().GetAuthUser();
	save->SetUserName(user ? user->Username : ByteString(""));
	save->SetID(0);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->paused = pausedCheckbox->GetChecked();
		save->SetGameSave(std::move(gameSave));
	}
	AddAuthorInfo();
	uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(*save);
	uploadSaveRequest->Start();
}

void ServerSaveActivity::Exit()
{
	WindowActivity::Exit();
}

void ServerSaveActivity::ShowPublishingInfo()
{
	String info =
		"在 The Powder Toy 中，在线存档分为“已发布”和“未发布”两种可见级别，可通过“发布”复选框选择。新存档默认不发布；不勾选时，其他人不会在存档列表中看到它。\n"
		"\n"
		"\bt已发布的存档\bw会出现在“按日期”列表中，其他用户可以查看、评论和评分；评分也会计入网站个人资料中公开显示的平均评分。\n"
		"\bt未发布的存档\bw不会出现在“按日期”列表中，也不计入平均评分。但它并非完全私密：知道存档 ID 的人仍可查看，因此可以只把 ID 分享给指定的人。\n"
		"\n"
		"要快速更新存档，请打开它并点击分栏保存按钮左侧的\bt“重新上传当前模拟”\bw。若要修改说明或发布状态，请点击右侧的\bt“修改模拟属性”\bw。存档名称无法修改；改名会创建一个与原存档分离、没有评论、评分或标签的新存档。\n"
		"完成作品后，可以打开存档并选择“修改模拟属性”来发布原本未发布的存档，或取消发布。也可在存档浏览器的“我的作品”分类中选中存档，再使用底部按钮\bt取消发布或删除\bw。\n"
		"发布不足一周且人气快速上升的存档会自动进入\bt首页\bw。只有已发布存档有此机会；管理员也可将存档推荐到首页，或将违规、不适合首页的存档移出。\n"
		"创建存档后可以反复更新，系统会保留一小段\bt历史版本\bw。在存档浏览器中右键单击存档并选择“查看历史”即可查看；误保存时可借此返回旧版本。\n"
		;

	new InformationMessage("发布说明", info, true);
}

void ServerSaveActivity::ShowRules()
{
	String rules =
		"\boS 部分：社交和社区规则\n"
		"\bw与社区互动时应遵循一些规则。这些规则由工作人员执行，任何与违反这些规则相关的问题都可能会由用户提请我们注意。本部分适用于保存上传、评论区、论坛、社区其他区域。\n"
		"\n"
		"\bt1。尝试使用正确的语法。\bw 英语是官方社区语言，但在区域或文化群体中不要求使用。如果您英文写得不好，我们建议您使用谷歌翻译。\n"
		"\bt2。不要发送垃圾邮件。\bw 这里没有一个一刀切的定义，但这个想法通常是显而易见的。此外，以下内容被视为垃圾邮件，可能会被隐藏或删除：\n"
		   "- 在同一主题上发布多个主题。尝试将游戏反馈或建议的线程合并为一个线程。\n"
		   "- 通过回复来顶撞旧线程。这就是我们所说的“死灵”或“死灵”。线程的内容可能已经过时（解决问题、想法等）。我们建议发布一个新线程以获取更新或更新的响应。\n"
		   "- 在主题上发布“+1”或其他简短回复。没有必要不断地碰撞话题并使寻找回复变得困难。回复非常适合提供建设性反馈，而“+1”按钮则表示您对内容的支持。\n"
		   "- 评论过长或乱码。诸如重复同一封信或几乎没有预期目的之类的评论都属于此规则的范围。使用不同语言的评论除外。\n"
		   "- 过度格式化。 UPPERCASE，粗体和斜体适度使用会很好，但请不要在整个帖子中使用它们。\n"
		"\bt3。尽量减少脏话。\bw 包含脏话的评论或保存有被删除的风险。这还包括用其他语言说脏话。\n"
		"\bt4。请勿上传露骨、攻击性或其他不当材料。\bw\n"
		   "- 这些包括但不限于：性、毒品、种族主义、过度政治或任何冒犯或侮辱一群人的内容。\n"
		   "- 也禁止以其他语言引用这些主题。请勿尝试绕过此规则。\n"
		   "- 禁止发布违反此规则的 URL 或图像。这包括您的个人资料信息中的链接或文本。\n"
		"\bt5。请勿在与 The Powder Toy 无关的第三方游戏、网站或其他地方做广告。\bw\n"
		   "- 该规则主要是为了防止人们浏览和宣传自己的游戏和产品。\n"
		   "- 禁止未经授权或非官方的社区聚会场所，例如Discord。\n"
		"\bt6。不允许进行拖钓。\bw 与某些规则一样，没有明确的定义。反复恶搞的用户比其他人更有可能被封禁，并且收到的封禁时间也更长。\n"
		"\bt7。请勿冒充任何人。\bw 禁止故意使用与我们社区或其他在线社区中其他用户相似的名称注册帐户。\n"
		"\bt8。请勿发布有关版主决定或问题的帖子。\bw 如果存在有关帐户被禁止或内容删除的问题，请通过消息系统联系版主。否则，应避免讨论主持人的行为。\n"
		"\bt9。避免后座调节。\bw 调节者是做出决定的人。用户应避免威胁禁止或违反规则可能造成的后果。如果存在可能的问题或您不确定，我们建议通过“报告”按钮或通过网站上的消息系统报告问题。\n"
		"\bt10。禁止纵容违反共同法律的行为。\bw 哪个国家的法律适用的管辖权尚不清楚，但有一些共同的法律可以了解。这些包括但不限于：\n"
		   "- 软件、音乐、百吉饼等盗版\n"
		   "- 黑客/窃取帐户\n"
		   "- 盗窃/欺诈\n"
		"\bt11。请勿跟踪或骚扰任何用户。\bw 近年来，通过不同的方法，这个问题已成为一个日益严重的问题，但通常包括：\n"
		   "- “人肉搜索”用户以查找他们的居住地或真实身份\n"
		   "- 当用户希望避免任何联系时不断向用户发送消息\n"
		   "- 大量否决可节省\n"
		   "- 对某人的内容（保存、论坛主题等）发表粗鲁或不必要的评论\n"
		   "- 强迫一组用户“瞄准”某个用户\n"
		   "- 个人争论或仇恨。这可能是在评论中争论或进行仇恨保存\n"
		   "- 一般而言，对人的歧视。这可能是宗教、种族等。\n"
		"\n"
		"\boG部分：游戏规则\n"
		"\bw这部分规则重点关注游戏中的动作。虽然S节也适用于游戏中，但以下规则更专门用于游戏中的社区互动。\n"
		"\bt1。请勿夺取他人的作品。\bw 这可能只是重新上传其他用户的作品或利用大部分保存内容。在正确使用的情况下，允许衍生作品。如果您使用某人的作品，默认情况下您必须注明作者。除非作者明确指出不同的使用条款，否则这是标准政策。衍生作品的特点是创新用法和原创性百分比（即原创性与某人的作品相比有多少？）。被盗的保存将被取消发布或禁用。\n"
		"\bt2。不允许自行投票或投票欺诈。\bw 定义为使用多个帐户对自己的保存或他人的保存进行投票。我们严格执行这条规则，因此，您必须明白，成功的禁令上诉很少。请确保您和其他帐户不是来自同一家庭投票。所有备用帐户将被永久禁止，主帐户将被暂时禁止，任何受影响的保存都将被禁用。\n"
		"\bt3。要求任何形式的投票都是不被允许的。\bw 这样做的保存将不会发布，直到问题得到解决。符合此规则的示例有：\n"
		   "- 可能暗示投票赞成或反对的迹象。签名绿色箭头或要求投票属于此规则。\n"
		   "- 要求投票的噱头。这些可能是交换某些东西的总票数，例如“100 票，我会制作一个更好的版本”。这就是我们定义的投票耕作。不允许任何类型的投票耕作。\n"
		   "- 禁止以使用保存或任何其他理由索取选票。\n"
		"\bt4。请勿发送垃圾邮件。\bw 如前所述，对于垃圾邮件的定义并没有标准。以下是一些可能属于垃圾邮件的示例：\n"
		   "- 在短时间内上传或重新上传类似的保存。不要试图绕过系统让人们看到/投票你的保存。这包括毫无目的地上传“垃圾”或“空白”保存。这些保存将被取消发布。\n"
		   "- 上传纯文本保存。这些可能是公告或寻求帮助之类的。我们的论坛和评论区可用于多种用途，这些纯文本保存可以满足多种目的。这些保存将从首页删除。\n"
		   "- 上传艺术保存并不严格禁止，但可能会导致首页降级。我们希望看到以创造性的方式使用各种元素。缺乏这些因素（例如仅装饰保存）通常会导致首页降级\n"
		"\bt5。避免上传露骨的色情或其他不适当的材料。这些保存将被删除并导致封禁。\bw\n"
		   "- 这些包括但不限于：性、毒品、种族主义、过度政治或任何冒犯或侮辱一群人的内容。\n"
		   "- 不要试图规避这条规则。任何通过直接或间接方式有意引用这些概念/想法的事物都属于此规则的范围。\n"
		   "- 也禁止以其他语言引用这些主题。请勿尝试绕过此规则。\n"
		   "- 禁止发布违反此规则的 URL 或图像。这包括您的个人资料信息中的链接或文本。\n"
		"\bt6。严格禁止绘制图像。\bw 这包括使用脚本或任何第三方工具为您绘制或创建保存。使用 CGI 进行的保存将被删除，您可能会收到禁令。\n"
		"\bt7。将徽标和标志保持在最低限度。\bw 这些保存可能会从首页删除。此规则限制的项目有：\n"
		   "- 放置过多标志\n"
		   "- 无预期用途的标志\n"
		   "- 虚假更新或通知标志\n"
		   "- 链接其他没有相关目的的保存\n"
		"\bt8。不要放置偏离主题或不适当的标签。\bw 标签仅用于改善搜索结果。它们通常应该只是保存的一个单词描述。句子或主观标签可能会被删除。不适当或令人反感的标签可能会让您被禁止。\n"
		"\bt9。故意导致延迟或崩溃的保存是被禁止的。\bw 如果大多数用户都在写有关导致崩溃或延迟的保存，那么该保存将属于此规则。这些保存将从首页删除或禁用。\n"
		"\bt10。不要滥用报告系统。\bw 发送诸如“保存错误”或乱码之类的报告原因会浪费我们的时间。除非问题涉及可能的违规行为或社区问题，否则请不要发送报告。如果您认为保存违规或造成社区问题，请发送报告！如果您善意地报告保存，则永远不会发生封禁。\n"
		"\bt11。不要要求将存档降级或从首页删除。\bw 除非存档违反任何规则，否则它将保留在首页。对于艺术保存来说，这条规则没有例外，也请不要报告艺术。\n"
		"\n"
		"\boR段：其他\n"
		"\bw主持人可以按照他们认为合适的方式解释这些规则。并非所有规则都是平等的，有些规则的执行力度比其他规则要少。版主对哪些内容违反规则、哪些内容不违反规则做出最终决定，但我们已尽最大努力涵盖此处的所有不良行为。每当规则更新时都会在此帖子中发布通知。\n"
		"\n"
		"违反这些规则可能会导致删除帖子/评论、取消发布或禁用保存、从首页删除保存，或者在更极端的情况下，导致临时或永久禁令。有各种手动和自动措施来执行这些规则。主持人之间的严重性和由此产生的决定可能不一致。\n"
		"\n"
		"如果您对哪些内容违反规则、哪些内容不违反规则有任何疑问，请随时联系版主。";

	new InformationMessage("存档上传规则", rules, true);
}

void ServerSaveActivity::CheckName(String newname)
{
	auto user = Client::Ref().GetAuthUser();
	if (newname.length() && newname == save->GetName() && user && save->GetUserName() == user->Username)
		titleLabel->SetText("修改模拟属性：");
	else
		titleLabel->SetText("上传新模拟：");
}

void ServerSaveActivity::OnTick()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Poll();
		if (thumbnailRenderer->GetDone())
		{
			thumbnail = thumbnailRenderer->Finish();
			thumbnailRenderer = nullptr;
		}
	}

	if (uploadSaveRequest && uploadSaveRequest->CheckDone())
	{
		okayButton->Enabled = true;
		try
		{
			save->SetID(uploadSaveRequest->Finish());
			Exit();
			new SaveIDMessage(save->GetID());
			if (onUploaded)
			{
				onUploaded(std::move(save));
			}
		}
		catch (const http::RequestError &ex)
		{
			new ErrorMessage("错误", "上传失败：\n" + ByteString(ex.what()).FromUtf8());
		}
		uploadSaveRequest.reset();
	}

	if(saveUploadTask)
		saveUploadTask->Poll();
}

void ServerSaveActivity::OnDraw()
{
	Graphics * g = GetGraphics();
	g->BlendRGBAImage(saveToServerImage->data(), RectSized(Vec2(-10, 0), saveToServerImage->Size()));
	g->DrawFilledRect(RectSized(Position, Size).Inset(-1), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);

	if (Size.X > 220)
		g->DrawLine(Position + Vec2(Size.X / 2 - 1, 0), Position + Vec2(Size.X / 2 - 1, Size.Y - 1), 0xFFFFFF_rgb);

	if (thumbnail)
	{
		auto rect = RectSized(Position + Vec2(Size.X / 2 + (Size.X / 2 - thumbnail->Size().X) / 2, 25), thumbnail->Size());
		g->BlendImage(thumbnail->Data(), 0xFF, rect);
		g->DrawRect(rect, 0xB4B4B4_rgb);
	}
}

ServerSaveActivity::~ServerSaveActivity()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Abandon();
	}
	delete saveUploadTask;
}
