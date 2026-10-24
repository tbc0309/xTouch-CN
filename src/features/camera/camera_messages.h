#pragma once
/* Short camera status captions, in the application's language order. */
static const char *const cameraMessages[][8]={
 {"连接摄像头…","連接攝影機…","Connecting…","Connexion…","Verbinden…","Подключение…","接続中…","연결 중…"},
 {"摄像头连接失败","攝影機連線失敗","Camera connection failed","Échec de connexion caméra","Kameraverbindung fehlgeschlagen","Ошибка подключения камеры","カメラ接続失敗","카메라 연결 실패"},
 {"摄像头数据超时","攝影機資料逾時","Camera timed out","Délai caméra dépassé","Kamera-Zeitüberschreitung","Тайм-аут камеры","カメラ応答なし","카메라 시간 초과"},
 {"图像格式不支持","影像格式不支援","Unsupported image","Image non prise en charge","Bildformat nicht unterstützt","Формат не поддерживается","画像形式に未対応","지원하지 않는 이미지"},
 {"需要局域网 IP 和访问码","需要區網 IP 與存取碼","LAN IP and code required","IP LAN et code requis","LAN-IP und Code nötig","Нужны LAN-IP и код","LAN IPとコードが必要","LAN IP 및 코드 필요"},
 {"摄像头可用内存不足","攝影機可用記憶體不足","Not enough camera memory","Mémoire caméra insuffisante","Zu wenig Kameraspeicher","Недостаточно памяти","カメラのメモリ不足","카메라 메모리 부족"},
 {"摄像头","攝影機","Camera","Caméra","Kamera","Камера","カメラ","카메라"},
 {"H.264 编码不支持","H.264 編碼不支援","Unsupported H.264 codec","Codec H.264 non pris en charge","H.264-Codec nicht unterstützt","Кодек H.264 не поддерживается","H.264形式に未対応","H.264 코덱 미지원"},
 {"重新连接摄像头…","重新連接攝影機…","Reconnecting…","Reconnexion…","Neu verbinden…","Переподключение…","再接続中…","다시 연결 중…"},
 {"画面已暂停","畫面已暫停","View paused","Image en pause","Bild angehalten","Просмотр приостановлен","映像を一時停止","화면 일시 정지"},
 {"未知摄像头协议","未知攝影機協定","Unknown camera protocol","Protocole caméra inconnu","Kameraprotokoll unbekannt","Неизвестный протокол","カメラ方式が不明","알 수 없는 카메라 프로토콜"},
 {"摄像头处理超时","攝影機處理逾時","Camera processing timed out","Délai de traitement caméra dépassé","Zeitlimit der Kameraverarbeitung","Тайм-аут обработки камеры","カメラ処理がタイムアウト","카메라 처리 시간 초과"},
 {"摄像头认证失败","攝影機驗證失敗","Camera authentication failed","Échec authentification caméra","Kamera-Anmeldung fehlgeschlagen","Ошибка входа в камеру","カメラ認証失敗","카메라 인증 실패"}
};
inline const char *cameraMessage(unsigned state,unsigned language){return cameraMessages[state<13?state:6][language<8?language:2];}
