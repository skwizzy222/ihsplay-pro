#include "error_messages.h"

const char *authorization_result_str(IHS_AuthorizationResult result) {
    switch (result) {
        case IHS_AuthorizationDenied:
            return "Хост отклонил авторизацию";
        case IHS_AuthorizationNotLoggedIn:
            return "Steam не выполнен вход на хосте";
        case IHS_AuthorizationOffline:
            return "Хост не в сети";
        case IHS_AuthorizationBusy:
            return "Хост занят — попробуйте чуть позже";
        case IHS_AuthorizationTimedOut:
            return "Время авторизации истекло";
        case IHS_AuthorizationCanceled:
            return "Авторизация отменена";
        case IHS_AuthorizationFailed:
            return "Ошибка авторизации";
        default:
            return "Неизвестная ошибка авторизации";
    }
}

const char *streaming_result_str(IHS_StreamingResult result) {
    switch (result) {
        case IHS_StreamingUnauthorized:
            return "Устройство не авторизовано — нужна привязка";
        case IHS_StreamingScreenLocked:
            return "Экран хоста заблокирован — разблокируйте и повторите";
        case IHS_StreamingFailed:
            return "Стрим не удалось запустить на хосте";
        case IHS_StreamingBusy:
            return "Хост занят другим стримом";
        case IHS_StreamingCanceled:
            return "Подключение отменено";
        case IHS_StreamingDriversNotInstalled:
            return "На хосте не установлены драйверы стрима";
        case IHS_StreamingDisabled:
            return "Remote Play отключён в настройках Steam";
        case IHS_StreamingBroadcastingActive:
            return "На хосте уже идёт трансляция";
        case IHS_StreamingVRActive:
            return "На хосте активна VR-сессия";
        case IHS_StreamingPINRequired:
            return "Требуется PIN для стрима";
        case IHS_StreamingTransportUnavailable:
            return "Сетевой транспорт недоступен";
        case IHS_StreamingInvisible:
            return "Хост не виден для Remote Play";
        case IHS_StreamingGameLaunchFailed:
            return "Хост не смог запустить игру";
        case IHS_StreamingTimeout:
            return "Нет ответа от хоста — проверьте сеть / firewall";
        default:
            return "Неизвестная ошибка стрима";
    }
}
