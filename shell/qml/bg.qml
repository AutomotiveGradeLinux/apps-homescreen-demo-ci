import QtQuick 2.2
import QtQuick.Window 2.11
import QtQuick.Layouts 1.1

Window {
	visible: true
	flags: Qt.FramelessWindowHint

	//width: Screen.width
	width: 300
	//height: Screen.height
	height: 400

	title: 'HomeScreen bg'

	color: 'red'

	property string appid: 'naq-activate'

	MouseArea {
		anchors.fill: parent
		onClicked: {
			console.log('activating ' + appid)
			shell.activate_app(Window.window, appid)
		}
	}
}
