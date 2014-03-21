
#ifndef JPEG_MARKERS_HPP_
#define JPEG_MARKERS_HPP_

namespace jpeg_marker
{
	enum jpeg_marker_e
	{
		START_OF_FRAME_BASELINE_DCT = 0xC0,
		START_OF_FRAME_PROGRESSIVE_DCT = 0xC2,
		HUFFMAN_TABLE = 0xC4,
		RESTART_BASE = 0xD0, // RSTn where n goes from 0 to 7 (0xD0 - 0xD7)
		START_OF_IMAGE = 0xD8,
		END_OF_IMAGE = 0xD9,
		START_OF_SCAN = 0xDA,
		QUANTIZATION_TABLE = 0xDB,
		RESTART_INTERVAL = 0xDD,

		APPLICATION_BASELINE = 0xE0, // APPn where n goes from 0 to 7 (0xE0 - 0xE7)
		JFIF = 0xE0, // APP0 = JFIF
		EXIF = 0xE1, // APP1 = EXIF
		COMMENT = 0xFE, // Plain text comment
		MARKER = 0xFF
	};
}

#endif /* JPEG_MARKERS_HPP_ */
