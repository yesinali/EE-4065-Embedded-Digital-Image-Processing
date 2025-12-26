import numpy as np

def load_images(path):
    with open(path, "rb") as f:
        # İlk 16 byte'ı (header) atla ve geri kalan veriyi 28x28 olarak oku
        buffer = f.read()[16:]
        images = np.frombuffer(buffer, dtype=np.uint8).reshape(-1, 28, 28)
    return images

def load_labels(path):
    with open(path, "rb") as f:
        # İlk 8 byte'ı (header) atla ve etiketleri oku
        buffer = f.read()[8:]
        labels = np.frombuffer(buffer, dtype=np.uint8)
    return labels