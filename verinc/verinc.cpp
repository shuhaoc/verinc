// verinc.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

void loadFile(wstring filePath, vector<wstring>& lines) {
	HANDLE file = ::CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file != INVALID_HANDLE_VALUE) {
		DWORD fileSize = ::GetFileSize(file, NULL);
		wchar_t* buffer = new wchar_t[fileSize / 2];
		DWORD readSize = 0;
		::SetFilePointer(file, 2, NULL, FILE_BEGIN);
		if (::ReadFile(file, buffer, fileSize - 2, &readSize, NULL)) {
			buffer[readSize / 2] = 0;
			wistringstream ss(buffer);

			wstring line;
			while (ss.good()) {
				std::getline(ss, line);
				lines.push_back(line);
			}
		}
		delete[] buffer;
	}
	::CloseHandle(file);
}

void saveFile(wstring filePath, const vector<wstring> lines) {
	HANDLE file = ::CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file != INVALID_HANDLE_VALUE) {
		DWORD writtenSize = 0;
		BOOL succ = ::WriteFile(file, "\xFF\xFE", 2, &writtenSize, NULL) && writtenSize == 2;
		if (succ) {
			for (unsigned i = 0; succ && i < lines.size(); i++) {
				// 空行应为\r，此判断是为了解决每更新一次，末尾多出一空行的问题
				if (lines[i].size() > 0) {
					wstring line = lines[i] + L"\n";
					if (i != line.size() - 1 || (line != L"\n" && line != L"\r\n")) {
						succ = ::WriteFile(file, line.data(), line.size() * 2, &writtenSize, NULL)
							&& writtenSize == line.size() * 2;
					}
				}
			}
		}
	}
	::CloseHandle(file);
}

template <typename CharT>
vector<basic_string<CharT> > splitString(const basic_string<CharT>& str, CharT delim) {
	vector<basic_string<CharT> > splits;
	int i = 0, j = 0;
	while ((j = str.find(delim, i)) != basic_string<CharT>::npos) {
		splits.push_back(str.substr(i, j - i));
		i = j + 1;
	}
	splits.push_back(str.substr(i));
	return splits;
}

template <typename CharT>
basic_string<CharT> joinString(const vector<basic_string<CharT> >& parts, CharT delim) {
	basic_stringstream<CharT> joined;
	if (parts.size() > 0) {
		for (unsigned i = 0; i < parts.size() - 1; i++) {
			joined << parts[i] << delim;
		}
		joined << *parts.rbegin();
	}
	return joined.str();
}

void increaseVersion(wstring& version, wchar_t delim) {
	vector<wstring> parts = splitString(version, delim);
	if (parts.size() == 4) {
		unsigned last = boost::lexical_cast<unsigned>(parts[3]);
		last++;
		parts[3] = boost::lexical_cast<wstring>(last);
		version = joinString(parts, delim);
	}
}

void increaseVersions(vector<wstring>& lines) {
	for (auto i = lines.begin(); i != lines.end(); i++) {
		wstring& line = *i;
		if (line.find(L"FILEVERSION") != wstring::npos
			|| line.find(L"PRODUCTVERSION") != wstring::npos) {
			// 找最后的空格
			unsigned lastCtrl = line.find_last_of(L'\r');
			unsigned lastSpace = line.find_last_of(L' ');
			if (lastCtrl != wstring::npos && lastSpace != wstring::npos
				&& lastCtrl > lastSpace) {
				wstring version = line.substr(lastSpace + 1, lastCtrl - lastSpace - 1);
				increaseVersion(version, L',');
				
				wcout << line << endl;
				line = line.substr(0, lastSpace) + L" " + version + L"\r";
				wcout << L" => " << version << endl;
			}
		}
		if (line.find(L"FileVersion") != wstring::npos
			|| line.find(L"ProductVersion") != wstring::npos) {
			// 找最后两个双引号
			unsigned lastQuote = line.find_last_of(L'"');
			unsigned lastButOneQuote = line.substr(0, lastQuote).find_last_of(L'"');
			if (lastQuote != wstring::npos && lastButOneQuote != wstring::npos
				&& lastQuote > lastButOneQuote) {
				wstring version = line.substr(lastButOneQuote + 1, lastQuote - lastButOneQuote - 1);
				increaseVersion(version, L'.');
				
				wcout << line << endl;
				line = line.substr(0, lastButOneQuote) + L"\"" + version + L"\"\r";
				wcout << L" => " << version << endl;
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2) return 1;

	vector<wstring> lines;
	wstring path = argv[1];
	loadFile(path, lines);
	increaseVersions(lines);	
	saveFile(path, lines);
	return 0;
}

