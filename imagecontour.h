#ifndef IMAGECONTOUR_H
#define IMAGECONTOUR_H

#include "preprocessing_config.h"

////////////////////////////////////////////////////////////////////////////////

class ImageContour : public cv::Mat1b {
public:
	enum {
		EMPTY = 0,
		CONTOUR = 255,
		INNER = 128
	};

	ImageContour() : cv::Mat1b(0, 0) {}

	//////////////////////////////////////////////////////////////////////////////

	//! build frtom the non zero pixels of \arg img, using C4 neigbourhood
	inline void from_image_C4(const cv::Mat1b & img) {
		from_image(img, false);
	}

	//////////////////////////////////////////////////////////////////////////////

	//! build frtom the non zero pixels of \arg img, using C8 neigbourhood
	inline void from_image_C8(const cv::Mat1b & img) {
		from_image(img, true);
	}

	//////////////////////////////////////////////////////////////////////////////

	//! compute contour size
	inline unsigned int contour_size() const {
		return cv::countNonZero(contour_image());
	}

	//////////////////////////////////////////////////////////////////////////////

	//! compute size of the inner non-zero pixels
	inline unsigned int inside_size() const {
		cv::Mat1b this_cp = *this;
		return cv::countNonZero(this_cp == INNER);
	}

	//////////////////////////////////////////////////////////////////////////////

	//! \return an image where pixels = 255 if on the contour, 0 otherwise
	inline cv::Mat1b contour_image() const {
		cv::Mat1b this_cp = *this;
		return (this_cp == CONTOUR);
	}

	//////////////////////////////////////////////////////////////////////////////

	//! returns reference to the specified element (2D case)
	inline uchar& operator ()(int row, int col) {
		return data[row * cols + col];
	}
	inline uchar& operator ()(int row, int col) const {
		return data[row * cols + col];
	}

	//////////////////////////////////////////////////////////////////////////////

	//! set a given point (row, col) as empty, with a C4 neighbourhood
	inline void set_point_empty_C4(int row, int col) {
		int key = row * cols + col;
		data[key] = EMPTY;
		if (col && data[key - 1] == INNER) // left
			data[key - 1] = CONTOUR;
		if (col < m_colsm && data[key + 1] == INNER) // right
			data[key + 1] = CONTOUR;
		if (row && data[key - cols] == INNER) // up
			data[key - cols] = CONTOUR;
		if (row < m_rowsm && data[key + cols] == INNER) // down
			data[key + cols] = CONTOUR;
	} // end set_point_empty();

	//////////////////////////////////////////////////////////////////////////////

	//! set a given point (row, col) as empty, with a C8 neighbourhood
	inline void set_point_empty_C8(int row, int col) {
		int key = row * cols + col;
		data[key] = EMPTY;
		bool left_ok = col, right_ok = col < m_colsm,
				up_ok = row, down_ok = row < m_rowsm;
		if (left_ok && up_ok && data[key - 1] == INNER) // left
			data[key - 1] = CONTOUR;
		if (right_ok && data[key + 1] == INNER) // right
			data[key + 1] = CONTOUR;
		if (up_ok && data[key - cols] == INNER) // up
			data[key - cols] = CONTOUR;
		if (down_ok && data[key + cols] == INNER) // down
			data[key + cols] = CONTOUR;

		// C8
		if (left_ok && up_ok && data[key - 1 - cols] == INNER) // left - up
			data[key - 1 - cols] = CONTOUR;
		if (right_ok && up_ok && data[key + 1 - cols] == INNER) // right - up
			data[key + 1 - cols] = CONTOUR;
		if (left_ok && down_ok && data[key - 1 + cols] == INNER) // left - down
			data[key - 1 + cols] = CONTOUR;
		if (right_ok && down_ok && data[key + 1 + cols] == INNER) // right - down
			data[key + 1 + cols] = CONTOUR;
	} // end set_point_empty();


	//////////////////////////////////////////////////////////////////////////////

	//! \return a compact string representation of a given ImageContour-linke image
	static std::string to_string(const cv::Mat1b & img) {
		std::ostringstream ans;
		ans << "(" << img.cols << "x" << img.rows << ")" << std::endl;
		for (int row = 0; row < img.rows; ++row) {
			for (int col = 0; col < img.cols; ++col) {
				switch (img(row, col)) {
				case EMPTY:
					ans << '-';
					break;
				case CONTOUR:
					ans << 'X';
					break;
				default:
				case INNER:
					ans << 'O';
					break;
				} // end switch (img(row, col))
			}
			ans << std::endl;
		}
		return ans.str();
	}

	//////////////////////////////////////////////////////////////////////////////

	//! \return a compact string representation of the current contour and inner pixels
	std::string to_string() const {
		return to_string(*this);
	}

	//////////////////////////////////////////////////////////////////////////////

	/*! \return a color image where the contour, the inside and the zero pixels
   * are represented by custom colors */
	const cv::Mat3b & illus(cv::Vec3b contour_color = cv::Vec3b(0, 0, 255),
							cv::Vec3b inner_color = cv::Vec3b(128, 128, 128),
							cv::Vec3b empty_color = cv::Vec3b(0, 0, 0)) {
		m_illus.create(rows, cols);
		unsigned int npixels = cols * rows;
		const uchar* in_ptr = ptr<uchar>(0);
		cv::Vec3b* out_ptr = m_illus.ptr<cv::Vec3b>(0);
		for (unsigned int pix_idx = 0; pix_idx < npixels; ++pix_idx) {
			switch (*in_ptr++) {
			case EMPTY:
				*out_ptr = empty_color;
				break;
			case CONTOUR:
				*out_ptr = contour_color;
				break;
			default:
			case INNER:
				*out_ptr = inner_color;
				break;
			} // end switch (control)
			++out_ptr;
		} // end loop pix_idx
		return m_illus;
	} // end illus()

private:
	////////////////////////////////////////////////////////////////////////////////

	inline void from_image(const cv::Mat1b & img, bool C8 = false) {
		// printf("from_image(cols:%i, rows:%i)\n", img.cols, img.rows);
		create(img.rows, img.cols) ;
		if (cols * rows == 0) {
			printf("Empty image\n");
			return;
		}
		assert(img.isContinuous());
		assert(isContinuous());
		setTo(EMPTY);
		m_colsm = cols - 1;
		m_rowsm = rows - 1;
		const uchar *img_data = img.data, *img_ptr = img.data;
		uchar* out_ptr = ptr<uchar>(0);
		if (C8)
			from_image_loop_C8(img_data, img_ptr, out_ptr);
		else
			from_image_loop_C4(img_data, img_ptr, out_ptr);
	}

	////////////////////////////////////////////////////////////////////////////////

	inline void from_image_loop_C4(const uchar* img_data, const uchar* img_ptr,
								   uchar* out_ptr) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				//printf("row:%i, col:%i\n", row, col);
				if (*img_ptr) {
					int key = row * cols + col;
					if ((!col || col == m_colsm || !row || row == m_rowsm) // border
							|| (col            && *img_ptr != img_data[key - 1]) //left
							|| (col < m_colsm    && *img_ptr != img_data[key + 1]) // right
							|| (row            && *img_ptr != img_data[key - cols]) // up
							|| (row < m_rowsm    && *img_ptr != img_data[key + cols])) // down
					{ // contour
						// printf("row:%i, col:%i is contour!\n", row, col);
						*out_ptr = CONTOUR;
						//out_ptr[key] = CONTOUR;
					} // end if real contour
					else // inner point
						*out_ptr = INNER;
					//out_ptr[key] = INNER;
				} // end if (img_ptr)
				img_ptr++;
				out_ptr++;
			} // end loop col
		} // end loop row
	} // end from_image_loop_C4()

	////////////////////////////////////////////////////////////////////////////////

	inline void from_image_loop_C8(const uchar* img_data, const uchar* img_ptr,
								   uchar* out_ptr) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				//printf("row:%i, col:%i\n", row, col);
				if (*img_ptr) {
					int key = row * cols + col;
					bool left_ok = col, right_ok = col < m_colsm,
							up_ok = row, down_ok = row < m_rowsm;
					if ((!left_ok || !right_ok || !up_ok || !down_ok) // border
							|| (left_ok              && *img_ptr != img_data[key - 1]) // L
							|| (right_ok             && *img_ptr != img_data[key + 1]) // R
							|| (up_ok                && *img_ptr != img_data[key - cols]) // U
							|| (down_ok              && *img_ptr != img_data[key + cols]) // D
							|| (left_ok && up_ok     && *img_ptr != img_data[key - cols - 1]) // LU
							|| (left_ok && down_ok   && *img_ptr != img_data[key + cols - 1]) // LD
							|| (right_ok && up_ok    && *img_ptr != img_data[key - cols + 1]) // RU
							|| (right_ok && down_ok  && *img_ptr != img_data[key + cols + 1]) // RD
							)
					{ // contour
						// printf("row:%i, col:%i is contour!\n", row, col);
						*out_ptr = CONTOUR;
					} // end if real contour
					else // inner point
						*out_ptr = INNER;
				} // end if (img_ptr)
				img_ptr++;
				out_ptr++;
			} // end loop col
		} // end loop row
	} // end from_image_loop_C8()

	//////////////////////////////////////////////////////////////////////////////

	int m_rowsm, m_colsm;
	cv::Mat3b m_illus;
}; // end class Imagecontour

#endif // IMAGECONTOUR_H
