/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SYNEDITSTRINGLIST_H
#define SYNEDITSTRINGLIST_H

#include <QStringList>
#include "highlighter/base.h"
#include <QFontMetrics>
#include <QMutex>
#include <QVector>
#include <memory>
#include "MiscProcs.h"
#include "../utils.h"
#include "Types.h"

enum SynEditStringFlag {
    sfHasTabs = 0x0001,
    sfHasNoTabs = 0x0002,
    sfExpandedLengthUnknown = 0x0004
};

typedef int SynEditStringFlags;

struct SynDocumentLine {
  QString fString;
  void * fObject;
  SynRangeState fRange;
  int fColumns;  //

public:
  explicit SynDocumentLine();
};

typedef std::shared_ptr<SynDocumentLine> PSynDocumentLine;

typedef QVector<PSynDocumentLine> SynDocumentLines;

typedef std::shared_ptr<SynDocumentLines> PSynDocumentLines;

class SynDocument;

typedef std::shared_ptr<SynDocument> PSynDocument;

class QFile;

class SynDocument : public QObject
{  
    Q_OBJECT
public:
    explicit SynDocument(const QFont& font, QObject* parent=nullptr);

    int parenthesisLevels(int Index);
    int bracketLevels(int Index);
    int braceLevels(int Index);
    int lineColumns(int Index);
    int leftBraces(int Index);
    int rightBraces(int Index);
    int lengthOfLongestLine();
    QString lineBreak() const;
    SynRangeState ranges(int Index);
    void setRange(int Index, const SynRangeState& ARange);
    QString getString(int Index);
    int count();
    void* getObject(int Index);
    QString text();
    void setText(const QString& text);
    void setContents(const QStringList& text);
    QStringList contents();

    void putString(int Index, const QString& s, bool notify=true);
    void putObject(int Index, void * AObject);

    void beginUpdate();
    void endUpdate();

    int add(const QString& s);
    void addStrings(const QStringList& Strings);

    int getTextLength();
    void clear();
    void deleteAt(int Index);
    void deleteLines(int Index, int NumLines);
    void exchange(int Index1, int Index2);
    void insert(int Index, const QString& s);
    void insertLines(int Index, int NumLines);

    void loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding);
    void saveToFile(QFile& file, const QByteArray& encoding,
                    const QByteArray& defaultEncoding, QByteArray& realEncoding);
    int stringColumns(const QString& line, int colsBefore) const;
    int charColumns(QChar ch) const;

    bool getAppendNewLineAtEOF();
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    FileEndingType getFileEndingType();
    void setFileEndingType(const FileEndingType &fileEndingType);

    bool empty();

    void resetColumns();
    int tabWidth() const {
        return mTabWidth;
    }
    void setTabWidth(int newTabWidth);

    const QFontMetrics &fontMetrics() const;
    void setFontMetrics(const QFont &newFont);

public slots:
    void invalidAllLineColumns();

signals:
    void changed();
    void changing();
    void cleared();
    void deleted(int index, int count);
    void inserted(int index, int count);
    void putted(int index, int count);
protected:
    QString getTextStr() const;
    void setUpdateState(bool Updating);
    void insertItem(int Index, const QString& s);
    void addItem(const QString& s);
    void putTextStr(const QString& text);
    void internalClear();
private:
    bool tryLoadFileByEncoding(QByteArray encodingName, QFile& file);

private:
    SynDocumentLines mLines;

    //SynEdit* mEdit;

    QFontMetrics mFontMetrics;
    int mTabWidth;
    int mCharWidth;
    //int mCount;
    //int mCapacity;
    FileEndingType mFileEndingType;
    bool mAppendNewLineAtEOF;
    int mIndexOfLongestLine;
    int mUpdateCount;
    QMutex mMutex;

    int calculateLineColumns(int Index);
};

enum class SynChangeReason {
    crInsert,
    crDelete,
    crCaret, //just restore the Caret, allowing better Undo behavior
    crSelection, //restore Selection
    crNothing,
    crLeftTop,
    crLineBreak,
    crMoveSelectionUp,
    crMoveSelectionDown
  //several undo entries can be chained together via the ChangeNumber
  //see also TCustomSynEdit.[Begin|End]UndoBlock methods
//  crDeleteAfterCursor,
//  crLineBreak, crIndent, crUnindent,
//  crSilentDelete, crSilentDeleteAfterCursor,
//  crAutoCompleteBegin, crAutoCompleteEnd,
//  crPasteBegin, crPasteEnd, //for pasting, since it might do a lot of operations
//  crSpecial1Begin, crSpecial1End,
//  crSpecial2Begin, crSpecial2End,

//  crNothing,
//  crDeleteAll,

  };
class SynEditUndoItem {
private:
    SynChangeReason mChangeReason;
    SynSelectionMode mChangeSelMode;
    BufferCoord mChangeStartPos;
    BufferCoord mChangeEndPos;
    QStringList mChangeText;
    int mChangeNumber;
public:
    SynEditUndoItem(SynChangeReason reason,
        SynSelectionMode selMode,
        BufferCoord startPos,
        BufferCoord endPos,
        const QStringList& text,
        int number);

    SynChangeReason changeReason() const;
    SynSelectionMode changeSelMode() const;
    BufferCoord changeStartPos() const;
    BufferCoord changeEndPos() const;
    QStringList changeText() const;
    int changeNumber() const;
};
using PSynEditUndoItem = std::shared_ptr<SynEditUndoItem>;

class SynEditUndoList : public QObject {
    Q_OBJECT
public:
    explicit SynEditUndoList();

    void AddChange(SynChangeReason AReason, const BufferCoord& AStart, const BufferCoord& AEnd,
      const QStringList& ChangeText, SynSelectionMode SelMode);

    void AddGroupBreak();
    void BeginBlock();
    void Clear();
    void DeleteItem(int index);
    void EndBlock();
    SynChangeReason LastChangeReason();
    bool isEmpty();
    void Lock();
    PSynEditUndoItem PeekItem();
    PSynEditUndoItem PopItem();
    void PushItem(PSynEditUndoItem Item);
    void Unlock();

    bool CanUndo();
    int ItemCount();

    int maxUndoActions() const;
    void setMaxUndoActions(int maxUndoActions);
    bool initialState();
    PSynEditUndoItem item(int index);
    void setInitialState(const bool Value);
    void setItem(int index, PSynEditUndoItem Value);

    int blockChangeNumber() const;
    void setBlockChangeNumber(int blockChangeNumber);

    int blockCount() const;

    bool insideRedo() const;
    void setInsideRedo(bool insideRedo);

    bool fullUndoImposible() const;

signals:
    void addedUndo();
protected:
    void ensureMaxEntries();
protected:
    int mBlockChangeNumber;
    int mBlockCount;
    bool mFullUndoImposible;
    QVector<PSynEditUndoItem> mItems;
    int mLockCount;
    int mMaxUndoActions;
    int mNextChangeNumber;
    int mInitialChangeNumber;
    bool mInsideRedo;
};

using PSynEditUndoList = std::shared_ptr<SynEditUndoList>;

#endif // SYNEDITSTRINGLIST_H
