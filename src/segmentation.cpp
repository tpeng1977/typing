#include "segmentation.h"

QVector<Token> segment(const QString& text, const Dict& dict, int maxLen)
{
    QVector<Token> out;
    const int n = text.size();
    int i = 0;
    while (i < n) {
        bool matched = false;
        int max = (i + maxLen <= n) ? maxLen : (n - i);
        for (int L = max; L >= 2; --L) {
            QString w = text.mid(i, L);
            if (dict.hasWord(w)) {
                out.append({i, L, true});
                i += L;
                matched = true;
                break;
            }
        }
        if (!matched) {
            out.append({i, 1, false});
            i += 1;
        }
    }
    return out;
}
