/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#pragma once
#include "Core.h"
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "Advapi32.lib")

namespace Gammy {
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Threading;
	using namespace System::IO;
	
	/// <summary>
	/// Riepilogo per MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form 
	{

	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::TextBox^  tempBox;
	private: System::Windows::Forms::NotifyIcon^  trayIcon;
	private: System::Windows::Forms::Label^  arrow;
	private: System::Windows::Forms::ContextMenu^ contextMenu;
	private: System::Windows::Forms::MenuItem^ startupItem;
	private: String^ fileName = Application::StartupPath + "\\" + "GammySettings.cfg";
	private: System::Windows::Forms::Label^  label11;
	private: HWND hWnd;
	public:
		void updateLabels(USHORT labelValue, USHORT targetValue, size_t t, size_t const &stop, short sleeptime) {
			if (brightnessLabel->Visible) {
				while (labelValue != targetValue && stop == t)
				{
					if (labelValue < targetValue) ++labelValue;
					else --labelValue;

					this->brightnessLabel->Text = Convert::ToString(labelValue);

					if (labelValue == MIN_BRIGHTNESS || labelValue == MAX_BRIGHTNESS) return;
					Sleep(sleeptime);
				}
			}
		}

		void setUpdateTimeLabel()
		{
			this->label4->Text = "Update rate (" + Convert::ToString(UPDATE_TIME_MIN) + " - " + Convert::ToString(UPDATE_TIME_MAX) + " ms)";

			this->updateRateBox->Text = Convert::ToString(UPDATE_TIME_MS);
		}

		MyForm(void)
		{
			InitializeComponent();
			
			this->CheckForIllegalCrossThreadCalls = false;
		}
	protected:
		/// <summary>
		/// Pulire le risorse in uso.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TextBox^  offsetBox;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::TextBox^  maxBrightnessBox;
	private: System::Windows::Forms::TextBox^  minBrightnessBox;
	private: System::Windows::Forms::TextBox^  updateRateBox;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;
	private: System::Windows::Forms::TextBox^  thresholdBox;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::TextBox^  speedBox;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Label^  brightnessLabel;

	private: System::ComponentModel::IContainer^  components;
	private:
		/// <summary>
		/// Variabile di progettazione necessaria.
		/// </summary>

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Metodo necessario per il supporto della finestra di progettazione. Non modificare
		/// il contenuto del metodo con l'editor di codice.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			System::Windows::Forms::Label^  arrowDown;
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MyForm::typeid));
			System::Windows::Forms::Label^  label10;
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->maxBrightnessBox = (gcnew System::Windows::Forms::TextBox());
			this->offsetBox = (gcnew System::Windows::Forms::TextBox());
			this->minBrightnessBox = (gcnew System::Windows::Forms::TextBox());
			this->updateRateBox = (gcnew System::Windows::Forms::TextBox());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->tempBox = (gcnew System::Windows::Forms::TextBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->speedBox = (gcnew System::Windows::Forms::TextBox());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->thresholdBox = (gcnew System::Windows::Forms::TextBox());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->brightnessLabel = (gcnew System::Windows::Forms::Label());
			this->trayIcon = (gcnew System::Windows::Forms::NotifyIcon(this->components));
			this->arrow = (gcnew System::Windows::Forms::Label());
			this->label11 = (gcnew System::Windows::Forms::Label());
			arrowDown = (gcnew System::Windows::Forms::Label());
			label10 = (gcnew System::Windows::Forms::Label());
			this->tableLayoutPanel1->SuspendLayout();
			this->SuspendLayout();
			// 
			// arrowDown
			// 
			arrowDown->CausesValidation = false;
			arrowDown->Enabled = false;
			arrowDown->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"arrowDown.Image")));
			arrowDown->Location = System::Drawing::Point(133, 45);
			arrowDown->Name = L"arrowDown";
			arrowDown->Size = System::Drawing::Size(18, 13);
			arrowDown->TabIndex = 16;
			arrowDown->Visible = false;
			// 
			// label10
			// 
			label10->CausesValidation = false;
			label10->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"label10.Image")));
			label10->Location = System::Drawing::Point(193, 12);
			label10->Name = L"label10";
			label10->Size = System::Drawing::Size(18, 13);
			label10->TabIndex = 17;
			label10->Click += gcnew System::EventHandler(this, &MyForm::closeBtn_Click);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(8, 52);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(79, 13);
			this->label1->TabIndex = 1;
			this->label1->Text = L"Offset (0- 255)";
			this->label1->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label1->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label1->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(8, 27);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(138, 13);
			this->label2->TabIndex = 3;
			this->label2->Text = L"Max. brightness (64 - 255)";
			this->label2->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label2->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label2->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(8, 0);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(137, 13);
			this->label3->TabIndex = 2;
			this->label3->Text = L"Min. brightness (64 - 255)";
			this->label3->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label3->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label3->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(8, 158);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(134, 13);
			this->label4->TabIndex = 4;
			this->label4->Text = L"Update rate (10 - 500 ms)";
			this->label4->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label4->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label4->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// maxBrightnessBox
			// 
			this->maxBrightnessBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->maxBrightnessBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->maxBrightnessBox->ForeColor = System::Drawing::Color::White;
			this->maxBrightnessBox->Location = System::Drawing::Point(180, 30);
			this->maxBrightnessBox->MaxLength = 3;
			this->maxBrightnessBox->Name = L"maxBrightnessBox";
			this->maxBrightnessBox->Size = System::Drawing::Size(29, 15);
			this->maxBrightnessBox->TabIndex = 6;
			this->maxBrightnessBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// offsetBox
			// 
			this->offsetBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->offsetBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->offsetBox->ForeColor = System::Drawing::Color::White;
			this->offsetBox->Location = System::Drawing::Point(180, 55);
			this->offsetBox->MaxLength = 3;
			this->offsetBox->Name = L"offsetBox";
			this->offsetBox->Size = System::Drawing::Size(29, 15);
			this->offsetBox->TabIndex = 1;
			this->offsetBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// minBrightnessBox
			// 
			this->minBrightnessBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->minBrightnessBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->minBrightnessBox->ForeColor = System::Drawing::Color::White;
			this->minBrightnessBox->Location = System::Drawing::Point(180, 3);
			this->minBrightnessBox->MaxLength = 3;
			this->minBrightnessBox->Name = L"minBrightnessBox";
			this->minBrightnessBox->Size = System::Drawing::Size(29, 15);
			this->minBrightnessBox->TabIndex = 7;
			this->minBrightnessBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// updateRateBox
			// 
			this->updateRateBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->updateRateBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->updateRateBox->ForeColor = System::Drawing::Color::White;
			this->updateRateBox->Location = System::Drawing::Point(180, 161);
			this->updateRateBox->MaxLength = 4;
			this->updateRateBox->Name = L"updateRateBox";
			this->updateRateBox->Size = System::Drawing::Size(29, 15);
			this->updateRateBox->TabIndex = 8;
			this->updateRateBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// button1
			// 
			this->button1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom));
			this->button1->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->button1->ForeColor = System::Drawing::Color::WhiteSmoke;
			this->button1->Location = System::Drawing::Point(85, 251);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(61, 29);
			this->button1->TabIndex = 9;
			this->button1->Text = L"Apply";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			// 
			// tableLayoutPanel1
			// 
			this->tableLayoutPanel1->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->tableLayoutPanel1->ColumnCount = 2;
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				78.28054F)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				21.71946F)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				20)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				20)));
			this->tableLayoutPanel1->Controls->Add(this->tempBox, 1, 4);
			this->tableLayoutPanel1->Controls->Add(this->label3, 0, 0);
			this->tableLayoutPanel1->Controls->Add(this->minBrightnessBox, 1, 0);
			this->tableLayoutPanel1->Controls->Add(this->label2, 0, 1);
			this->tableLayoutPanel1->Controls->Add(this->maxBrightnessBox, 1, 1);
			this->tableLayoutPanel1->Controls->Add(this->label6, 0, 3);
			this->tableLayoutPanel1->Controls->Add(this->label1, 0, 2);
			this->tableLayoutPanel1->Controls->Add(this->speedBox, 1, 3);
			this->tableLayoutPanel1->Controls->Add(this->offsetBox, 1, 2);
			this->tableLayoutPanel1->Controls->Add(this->label9, 0, 4);
			this->tableLayoutPanel1->Controls->Add(this->label5, 0, 5);
			this->tableLayoutPanel1->Controls->Add(this->label4, 0, 6);
			this->tableLayoutPanel1->Controls->Add(this->thresholdBox, 1, 5);
			this->tableLayoutPanel1->Controls->Add(this->updateRateBox, 1, 6);
			this->tableLayoutPanel1->Location = System::Drawing::Point(2, 64);
			this->tableLayoutPanel1->Margin = System::Windows::Forms::Padding(0);
			this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
			this->tableLayoutPanel1->Padding = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->tableLayoutPanel1->RowCount = 7;
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 52.27273F)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 47.72727F)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 25)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 26)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 27)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 28)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 21)));
			this->tableLayoutPanel1->Size = System::Drawing::Size(231, 180);
			this->tableLayoutPanel1->TabIndex = 10;
			this->tableLayoutPanel1->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->tableLayoutPanel1->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->tableLayoutPanel1->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// tempBox
			// 
			this->tempBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->tempBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->tempBox->ForeColor = System::Drawing::Color::White;
			this->tempBox->Location = System::Drawing::Point(180, 106);
			this->tempBox->MaxLength = 3;
			this->tempBox->Name = L"tempBox";
			this->tempBox->Size = System::Drawing::Size(29, 15);
			this->tempBox->TabIndex = 14;
			this->tempBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(8, 77);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(70, 13);
			this->label6->TabIndex = 11;
			this->label6->Text = L"Speed (1 - 4)";
			this->label6->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label6->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label6->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// speedBox
			// 
			this->speedBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->speedBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->speedBox->ForeColor = System::Drawing::Color::White;
			this->speedBox->Location = System::Drawing::Point(180, 80);
			this->speedBox->MaxLength = 3;
			this->speedBox->Name = L"speedBox";
			this->speedBox->Size = System::Drawing::Size(29, 15);
			this->speedBox->TabIndex = 12;
			this->speedBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Location = System::Drawing::Point(8, 103);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(102, 13);
			this->label9->TabIndex = 13;
			this->label9->Text = L"Temperature (1 - 4)";
			this->label9->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label9->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label9->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(8, 130);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(101, 13);
			this->label5->TabIndex = 9;
			this->label5->Text = L"Threshold (0 - 255)";
			this->label5->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label5->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label5->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// thresholdBox
			// 
			this->thresholdBox->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(35)), static_cast<System::Int32>(static_cast<System::Byte>(57)),
				static_cast<System::Int32>(static_cast<System::Byte>(65)));
			this->thresholdBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->thresholdBox->ForeColor = System::Drawing::Color::White;
			this->thresholdBox->Location = System::Drawing::Point(180, 133);
			this->thresholdBox->MaxLength = 3;
			this->thresholdBox->Name = L"thresholdBox";
			this->thresholdBox->Size = System::Drawing::Size(29, 15);
			this->thresholdBox->TabIndex = 10;
			this->thresholdBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::keyPress);
			// 
			// label7
			// 
			this->label7->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->label7->Location = System::Drawing::Point(60, 22);
			this->label7->Margin = System::Windows::Forms::Padding(0);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(65, 13);
			this->label7->TabIndex = 11;
			this->label7->Text = L"Brightness:";
			this->label7->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			this->label7->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->label7->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->label7->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// brightnessLabel
			// 
			this->brightnessLabel->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->brightnessLabel->Font = (gcnew System::Drawing::Font(L"Segoe UI", 14));
			this->brightnessLabel->ForeColor = System::Drawing::Color::White;
			this->brightnessLabel->Location = System::Drawing::Point(121, 10);
			this->brightnessLabel->Margin = System::Windows::Forms::Padding(0);
			this->brightnessLabel->Name = L"brightnessLabel";
			this->brightnessLabel->Size = System::Drawing::Size(42, 33);
			this->brightnessLabel->TabIndex = 12;
			this->brightnessLabel->Text = L"255";
			this->brightnessLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			this->brightnessLabel->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->brightnessLabel->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->brightnessLabel->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			// 
			// trayIcon
			// 
			this->trayIcon->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"trayIcon.Icon")));
			this->trayIcon->Text = L"Gammy: -";
			this->trayIcon->Visible = true;
			this->trayIcon->Click += gcnew System::EventHandler(this, &MyForm::trayIcon_Click);
			this->trayIcon->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::trayIcon_MouseMove);
			// 
			// arrow
			// 
			this->arrow->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"arrow.Image")));
			this->arrow->Location = System::Drawing::Point(111, 45);
			this->arrow->Margin = System::Windows::Forms::Padding(0);
			this->arrow->Name = L"arrow";
			this->arrow->Size = System::Drawing::Size(14, 19);
			this->arrow->TabIndex = 15;
			this->arrow->Click += gcnew System::EventHandler(this, &MyForm::label10_Click);
			// 
			// label11
			// 
			this->label11->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"label11.Image")));
			this->label11->Location = System::Drawing::Point(172, 12);
			this->label11->Margin = System::Windows::Forms::Padding(0);
			this->label11->Name = L"label11";
			this->label11->Size = System::Drawing::Size(14, 19);
			this->label11->TabIndex = 18;
			this->label11->Click += gcnew System::EventHandler(this, &MyForm::label8_Click);
			// 
			// MyForm
			// 
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::None;
			this->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(15)), static_cast<System::Int32>(static_cast<System::Byte>(35)),
				static_cast<System::Int32>(static_cast<System::Byte>(45)));
			this->ClientSize = System::Drawing::Size(235, 285);
			this->Controls->Add(this->label11);
			this->Controls->Add(label10);
			this->Controls->Add(arrowDown);
			this->Controls->Add(this->arrow);
			this->Controls->Add(this->brightnessLabel);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->tableLayoutPanel1);
			this->Controls->Add(this->button1);
			this->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->ForeColor = System::Drawing::Color::WhiteSmoke;
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->MaximizeBox = false;
			this->Name = L"MyForm";
			this->Opacity = 0.5;
			this->Padding = System::Windows::Forms::Padding(2);
			this->StartPosition = System::Windows::Forms::FormStartPosition::Manual;
			this->Text = L"Gammy";
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			this->VisibleChanged += gcnew System::EventHandler(this, &MyForm::MyForm_VisibleChanged);
			this->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseDown);
			this->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseMove);
			this->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::FormMain_MouseUp);
			this->tableLayoutPanel1->ResumeLayout(false);
			this->tableLayoutPanel1->PerformLayout();
			this->ResumeLayout(false);

		}
#pragma endregion
	
	bool dragging;
	Point dragCursorPoint;
	Point dragFormPoint;

private: System::Void MyForm_Load(System::Object^  sender, System::EventArgs^  e) 
{
		this->SetWindowState(false);
		hWnd = static_cast<HWND>(Handle.ToPointer());

		this->minBrightnessBox->Text = Convert::ToString(MIN_BRIGHTNESS);
		this->maxBrightnessBox->Text = Convert::ToString(MAX_BRIGHTNESS);
		this->offsetBox->Text = Convert::ToString(OFFSET);
		this->speedBox->Text = Convert::ToString(SPEED);
		this->tempBox->Text = Convert::ToString(TEMP);
		this->thresholdBox->Text = Convert::ToString(THRESHOLD);
		this->updateRateBox->Text = Convert::ToString(UPDATE_TIME_MS);

		this->ClientSize = System::Drawing::Size(225, 285); //225, 60 to start with hidden options
		this->TopMost = true;
		this->Location = Point(w - this->Width, h - this->Height - 40);

		this->contextMenu = gcnew System::Windows::Forms::ContextMenu();
		startupItem = gcnew MenuItem("Run at startup", gcnew System::EventHandler(this, &MyForm::trayStartup_Click));
		
		contextMenu->MenuItems->Add(0, startupItem);
		contextMenu->MenuItems->Add(1, gcnew MenuItem("Exit Gammy", gcnew System::EventHandler(this, &MyForm::closeBtn_Click)));
		trayIcon->ContextMenu = contextMenu;

		LRESULT s = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Gammy", RRF_RT_REG_SZ, 0, 0, 0);
		if (s == ERROR_SUCCESS) startupItem->Checked = true;
	}
private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {

		unsigned short newMin, newMax, newOffset, newSpeed, newTemp, newThreshold, newUpdateRate;
		array<String^> ^arr = gcnew array<String^>(settingsCount);

		arr[0] = this->minBrightnessBox->Text;
		arr[1] = this->maxBrightnessBox->Text;
		arr[2] = this->offsetBox->Text;
		arr[3] = this->speedBox->Text;
		arr[4] = this->tempBox->Text;
		arr[5] = this->thresholdBox->Text;
		arr[6] = this->updateRateBox->Text;

		for (int i = 0; i < settingsCount; ++i) {
			if (String::IsNullOrEmpty(arr[i])) {
				this->minBrightnessBox->Text = Convert::ToString(MIN_BRIGHTNESS);
				this->maxBrightnessBox->Text = Convert::ToString(MAX_BRIGHTNESS);
				this->offsetBox->Text = Convert::ToString(OFFSET);
				this->speedBox->Text = Convert::ToString(SPEED);
				this->tempBox->Text = Convert::ToString(TEMP);
				this->thresholdBox->Text = Convert::ToString(THRESHOLD);
				this->updateRateBox->Text = Convert::ToString(UPDATE_TIME_MS);
				return;
			}
		}

		newMin		  = Convert::ToUInt16(arr[0]);
		newMax		  = Convert::ToUInt16(arr[1]);
		newOffset	  = Convert::ToUInt16(arr[2]);
		newSpeed	  = Convert::ToUInt16(arr[3]);
		newTemp		  = Convert::ToUInt16(arr[4]);
		newThreshold  = Convert::ToUInt16(arr[5]);
		newUpdateRate = Convert::ToUInt16(arr[6]);

		if (newMin > DEFAULT_BRIGHTNESS) newMin = DEFAULT_BRIGHTNESS;
		else if (newMin < MIN_BRIGHTNESS_LIMIT) newMin = MIN_BRIGHTNESS_LIMIT;
		
		if (newMax > DEFAULT_BRIGHTNESS) newMax = DEFAULT_BRIGHTNESS;
		else if (newMax < MIN_BRIGHTNESS_LIMIT) newMax = MIN_BRIGHTNESS_LIMIT;
		if (newMin > newMax) newMin = newMax;
		
		if (newOffset > DEFAULT_BRIGHTNESS) newOffset = DEFAULT_BRIGHTNESS;
		else if (newOffset < 0) newOffset = 0;
		
		if (newSpeed > 4) newSpeed = 4;
		else if (newSpeed < 1) newSpeed = 1;

		if (newTemp > 4) newTemp = 4;
		else if (newTemp < 1) newTemp = 1;
		
		if (newThreshold > DEFAULT_BRIGHTNESS) newThreshold = DEFAULT_BRIGHTNESS;
		else if (newThreshold < 0) newThreshold = 0;
		
		if (newUpdateRate > UPDATE_TIME_MAX) newUpdateRate = UPDATE_TIME_MAX;
		else if (newUpdateRate < UPDATE_TIME_MIN) newUpdateRate = UPDATE_TIME_MIN;

		MIN_BRIGHTNESS = Convert::ToByte(newMin);
		MAX_BRIGHTNESS = Convert::ToByte(newMax);
		OFFSET = newOffset;
		SPEED = Convert::ToByte(newSpeed);
		TEMP = Convert::ToByte(newTemp);
		THRESHOLD = Convert::ToByte(newThreshold);
		UPDATE_TIME_MS = newUpdateRate;

		this->minBrightnessBox->Text = Convert::ToString(MIN_BRIGHTNESS);
		this->maxBrightnessBox->Text = Convert::ToString(MAX_BRIGHTNESS);
		this->offsetBox->Text = Convert::ToString(OFFSET);
		this->speedBox->Text = Convert::ToString(SPEED);
		this->tempBox->Text = Convert::ToString(TEMP);
		this->thresholdBox->Text = Convert::ToString(THRESHOLD);
		this->updateRateBox->Text = Convert::ToString(UPDATE_TIME_MS);

		StreamWriter^ sw = gcnew StreamWriter(fileName, false);

		sw->WriteLine("minBrightness={0}", MIN_BRIGHTNESS);
		sw->WriteLine("maxBrightness={0}", MAX_BRIGHTNESS);
		sw->WriteLine("offset={0}", OFFSET);
		sw->WriteLine("speed={0}", SPEED);
		sw->WriteLine("temp={0}", TEMP);
		sw->WriteLine("threshold={0}", THRESHOLD);
		sw->WriteLine("updateRate={0}", UPDATE_TIME_MS);
		sw->Close();

		setGDIBrightness(res, gdivs[TEMP-1], bdivs[TEMP-1]);
	}
private: System::Void SetWindowState(bool visible)
{
	this->Opacity = visible ? 0.94f : 0;
	this->Visible = visible;
	this->ShowInTaskbar = visible;
}
private: System::Void FormMain_MouseDown(Object^  sender, MouseEventArgs^ e)
{
	dragging = true;
	dragCursorPoint = Cursor->Position;
	dragFormPoint = this->Location;
}
private: System::Void FormMain_MouseMove(Object^  sender, MouseEventArgs^ e)
{
	if (dragging)
	{
		Point^ dif = Point::Subtract(Cursor->Position, (System::Drawing::Size)(gcnew System::Drawing::Size(dragCursorPoint)));
		this->Location = (Point)Point::Add(dragFormPoint, (System::Drawing::Size)(gcnew System::Drawing::Size((Point)dif)));
	}
}
private: System::Void FormMain_MouseUp(Object^  sender, MouseEventArgs^ e)
{
	dragging = false;
}
private: System::Void closeBtn_Click(System::Object^  sender, System::EventArgs^  e) 
{
	this->Opacity = 0;
	trayIcon->Visible = false;
		
	setGDIBrightness(DEFAULT_BRIGHTNESS, 1, 1);

	this->Close();
}
private: System::Void label8_Click(System::Object^  sender, System::EventArgs^  e) {
	this->SetWindowState(false);
}		
private: System::Void trayIcon_Click(System::Object^  sender, System::EventArgs^  e) {
	this->SetWindowState(true);
}
private: System::Void label10_Click(System::Object^  sender, System::EventArgs^  e) 
{
	System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MyForm::typeid));

	if (this->tableLayoutPanel1->Visible) {
		this->tableLayoutPanel1->Visible = false;
		this->ClientSize = System::Drawing::Size(225, 60);
		this->arrow->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"arrowDown.Image")));
	}
	else {
		this->tableLayoutPanel1->Visible = true;
		this->ClientSize = System::Drawing::Size(225, 285);
		this->arrow->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"arrow.Image")));
	}
}	
private: System::Void trayExit_Click(System::Object^ sender, System::EventArgs^ e) {
		trayIcon->Visible = false;
		this->Close();
		}
private: System::Void trayStartup_Click(System::Object^ sender, System::EventArgs^ e) 
{
	startupItem->Checked = !startupItem->Checked;
	BOOL checked = startupItem->Checked;

	LSTATUS s;
	HKEY hKey;
	LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

	if (checked) 
	{
		WCHAR path[MAX_PATH + 3], tmpPath[MAX_PATH + 3];

		GetModuleFileName(0, tmpPath, MAX_PATH + 1);

		wsprintfW(path, L"\"%s\"", tmpPath);

		s = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &hKey, NULL);
			
		if (s == ERROR_SUCCESS) {
			#ifdef _db
			printf("RegKey created/opened. \n");
			#endif

			s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, (BYTE*)path, (int)(wcslen(path) * sizeof(WCHAR)));
				
			#ifdef _db
				if (s == ERROR_SUCCESS) {
					printf("RegValue set.\n");
				}
				else {
					printf("Error when setting RegValue.\n");
				}
			#endif
		}
		#ifdef _db
		else {
			printf("Error when creating/opening RegKey.\n");
		}
		#endif
	}
	else {
		s = RegDeleteKeyValueW(HKEY_CURRENT_USER, subKey, L"Gammy");

		#ifdef _db
			if (s == ERROR_SUCCESS) 
				printf("RegValue deleted.\n");
			else 
				printf("RegValue deletion failed.\n");
		#endif
	}

	if(hKey) RegCloseKey(hKey);
}
private: System::Void keyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^  e) 
	{
		if (!Char::IsControl(e->KeyChar) && !Char::IsDigit(e->KeyChar) && (e->KeyChar != '.')) e->Handled = true;
	}
private: System::Void trayIcon_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
	this->trayIcon->Text = "Gammy: " + Convert::ToString(res);
}
private: System::Void MyForm_Shown(System::Object^  sender, System::EventArgs^  e) {
	this->brightnessLabel->Text = Convert::ToString(res);
}
private: System::Void MyForm_VisibleChanged(System::Object^  sender, System::EventArgs^  e) {

	if (targetRes > DEFAULT_BRIGHTNESS) targetRes = DEFAULT_BRIGHTNESS;
	else if (targetRes < MIN_BRIGHTNESS) targetRes = MIN_BRIGHTNESS;

	this->brightnessLabel->Text = Convert::ToString(targetRes);
}


};//class
}//namespace