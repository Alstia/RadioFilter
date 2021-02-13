#include "RingBuffer.h"
#include "../../macro.h"

namespace Alstia {
	namespace Buffer {
		RingBuffer::RingBuffer(const unsigned int bufferSize)
		{
			//読み込み位置・書き込み位置を初期化
			rPos = 0;
			wPos = 0;

			//データ数初期化
			dataCnt = 0;

			//バッファサイズ代入
			buffSize = bufferSize;
			//バッファのメモリを確保
			buff = new float[buffSize];
		}

		RingBuffer::~RingBuffer()
		{
			//バッファのメモリを開放
			SAFE_DELETE_ARRAY(buff);
		}

		float RingBuffer::Read(const unsigned int pos)
		{
			//データ数チェック
			if (GetDataSize() == 0) return 0.0f;

			//指定位置のデータを読み取って返す
			return buff[(rPos + pos) % buffSize];
		}

		bool RingBuffer::Write(const float in)
		{
			//データ数チェック
			if (GetDataSize() == buffSize) return false;

			/* 書き込み */
			//データ書き込み
			buff[wPos] = in;
			//書き込み位置加算
			++wPos;
			//上限超過対策
			wPos %= buffSize;
			//データ数加算
			++dataCnt;

			return true;
		}

		void RingBuffer::Erase(unsigned int count)
		{
			//データ数取得
			const unsigned int dataSize = GetDataSize();

			//データ数チェック
			if (dataSize < count) {
				//データ数より削除指定数が多かったらデータ数にする
				count = dataSize;
			}

			//読み込み位置加算
			rPos += count;
			//上限超過チェック
			rPos %= buffSize;
			//データ数減算
			dataCnt -= count;
		}
	}
}