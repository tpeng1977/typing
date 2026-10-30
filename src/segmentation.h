#pragma once
#include <QString>
#include <QVector>
#include "dataloader.h"

// 一个被切分出来的“词/字”单元
struct Token {
    int start = 0;   // 在原文中的起始下标
    int len   = 1;   // 长度
    bool isWord = false; // 是否为一个多字词语
};

// 正向最大匹配分词：优先匹配词典中最长的词，否则退回单字
// maxLen 为可匹配的最大词长
QVector<Token> segment(const QString& text, const Dict& dict, int maxLen = 6);
