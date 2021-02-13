#pragma once

namespace Alstia {
	namespace Buffer {
		class RingBuffer /*: public std::enable_shared_from_this<RingBuffer> */
		{
		public:
			RingBuffer(const unsigned int bufferSize);
			~RingBuffer();


			/* バッファ管理 */
			//バッファサイズを返す
			inline unsigned int GetBufferSize() const { return buffSize; }
			//格納済みのデータ数を計算して返す
			inline unsigned int GetDataSize() const { return dataCnt; }


			/* バッファ使用 */
			// 内部バッファの読み込み位置(rpos)のデータを読み込む関数
			// 引数のposは読み込み位置(rpos)からの相対位置
			// (相対位置(pos)はコーラスやピッチシフタなどのエフェクターで利用する)
			float Read(const unsigned int pos = 0);

			// 内部バッファの書き込み位置(wpos)にデータを書き込む関数
			bool Write(const float in);

			//内部バッファの読み込み位置から指定量のデータを削除
			void Erase(unsigned int count = 1);

		private:
			unsigned int rPos;		//読み込み位置
			unsigned int wPos;		//書き込み位置

			unsigned int dataCnt;	//格納済みのデータ数

			unsigned int buffSize;	//内部バッファのサイズ
			float* buff;			//内部バッファ
		};
	}
}