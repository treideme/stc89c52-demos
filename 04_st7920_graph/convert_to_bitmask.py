import cv2
import numpy as np

def convert_image_to_bitmask_bytes(image_path, threshold=128):
  image = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
  if image is None:
    raise ValueError("Image not found or unable to load.")
  _, binary_image = cv2.threshold(image, threshold, 255, cv2.THRESH_BINARY)

  bits = (binary_image > 0).astype(np.uint8)
  h, w = bits.shape
  bytes_per_row = (w + 7) // 8
  packed = np.zeros((h, bytes_per_row), dtype=np.uint8)

  for r in range(h):
    row = bits[r]
    for bidx in range(bytes_per_row):
      start = bidx * 8
      val = 0
      for b in range(8):
        idx = start + b
        if idx < w and row[idx]:
          val |= (1 << (7 - b))
      # Invert bits for display purposes
      packed[r, bidx] = 0xFF-val
  return packed

if __name__ == "__main__":
  image_path = "cindy_crawford_helmut_newton_bitmask.png"
  bitmask = convert_image_to_bitmask_bytes(image_path)
  print("#ifndef CINDY_CRAWFORD_HELMUT_NEWTON_BITMASK_H")
  print("#define CINDY_CRAWFORD_HELMUT_NEWTON_BITMASK_H")
  print(f"#define CINDY_CRAWFORD_HELMUT_NEWTON_BITMASK_WIDTH {bitmask.shape[1] * 8}")
  print(f"#define CINDY_CRAWFORD_HELMUT_NEWTON_BITMASK_HEIGHT {bitmask.shape[0]}")
  print("const uint8_t cindy_crawford_helmut_newton_bitmask[] = {")
  for row in bitmask:
    row_data = ', '.join(f'0x{byte:02X}' for byte in row)
    print(f"  {row_data},")
  print("};")
  print("#endif // CINDY_CRAWFORD_HELMUT_NEWTON_BITMASK_H")